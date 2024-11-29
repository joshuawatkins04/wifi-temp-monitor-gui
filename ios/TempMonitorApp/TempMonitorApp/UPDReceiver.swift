import SwiftUI
import Network

class UDPReceiver: ObservableObject {
    @Published var temperature: String = "--"
    @Published var humidity: String = "--"
    @Published var connected: Bool = false

    private var listener: NWListener?
    private var multicastGroup: NWConnectionGroup?
    private var heartbeatTimer: Timer?
    private var failCount: Int = 0
    private let failThreshold: Int = 3
    private let heartbeatInterval: TimeInterval = 3

    // Multicast address and port
    private let multicastHost = NWEndpoint.Host("224.0.0.251") // Example multicast address
    private let multicastPort = NWEndpoint.Port(5353)! // Example multicast port

    // ESP device endpoint
    private var espHost: NWEndpoint.Host?
    private var espPort: NWEndpoint.Port?

    init(port: UInt16 = 12345) {
        startListening(port: port)
        startMulticastListening()
    }

    func startListening(port: UInt16) {
        do {
            let params = NWParameters.udp
            listener = try NWListener(using: params, on: NWEndpoint.Port(rawValue: port)!)
            listener?.stateUpdateHandler = { state in
                print("Listening state: \(state)")
            }
            listener?.newConnectionHandler = { connection in
                connection.stateUpdateHandler = { newState in
                    print("Connection state: \(newState)")
                    if case .failed(let error) = newState {
                        print("Connection failed with error: \(error)")
                    }
                }
                connection.start(queue: .main)
                self.receive(on: connection)
            }
            listener?.start(queue: .main)
        } catch {
            print("Failed to start listener: \(error)")
        }
    }

    func startMulticastListening() {
        let multicastGroupEndpoint = NWMulticastGroup(for: [.hostPort(host: multicastHost, port: multicastPort)])
        let params = NWParameters.udp
        params.allowLocalEndpointReuse = true // Allows multiple connections on the same port
        multicastGroup = NWConnectionGroup(with: multicastGroupEndpoint, using: params)
        multicastGroup?.setReceiveHandler(maximumMessageSize: 65536, rejectOversizedMessages: true) { [weak self] (message, data, isComplete) in
            if let data = data, let messageString = String(data: data, encoding: .utf8) {
                print("Received multicast message: \(messageString)")
                self?.handleMulticastMessage(messageString, from: message.remoteEndpoint)
            }
        }
        multicastGroup?.stateUpdateHandler = { newState in
            print("Multicast group state: \(newState)")
        }
        multicastGroup?.start(queue: .main)
    }

    private func receive(on connection: NWConnection) {
        connection.receiveMessage { (data, context, isComplete, error) in
            if let data = data, let message = String(data: data, encoding: .utf8) {
                let remoteEndpoint = connection.endpoint
                print("Received from \(remoteEndpoint): \(message)")
                self.handleMessage(message, from: connection)
            }
            if let error = error {
                print("Error receiving data: \(error)")
                return
            }
            self.receive(on: connection)
        }
    }

    private func handleMulticastMessage(_ message: String, from endpoint: NWEndpoint) {
        if message == "DISCOVERY_REQUEST" {
            print("Received DISCOVERY_REQUEST from \(endpoint)")
            sendResponse("IOS_RESPONSE", to: endpoint)
        } else {
            print("Unknown multicast message: \(message)")
        }
    }

    private func handleMessage(_ message: String, from connection: NWConnection) {
        if case let NWEndpoint.hostPort(host, port) = connection.endpoint {
            self.espHost = host
            self.espPort = port
            print("ESP IP: \(host), Port: \(port)")
        }

        let heartbeatAckMessage = "HEARTBEAT_ACK"
        let heartbeatMessage = "HEARTBEAT"

        if message == heartbeatMessage {
            print("Heartbeat received from ESP. Sending ACK...")
            self.failCount = 0
            self.sendResponse(heartbeatAckMessage, to: connection.endpoint)
        } else if message.contains(",") {
            self.connected = true
            self.failCount = 0
            if self.heartbeatTimer == nil {
                self.startHeartbeatTimer()
            }
            self.parseMessage(message)
        } else {
            print("Unknown message received: \(message)")
        }
    }

    private func sendResponse(_ responseMessage: String, to endpoint: NWEndpoint) {
        print("Sending response to \(endpoint): \(responseMessage)")
        let connection = NWConnection(to: endpoint, using: .udp)
        connection.stateUpdateHandler = { newState in
            switch newState {
            case .ready:
                let responseData = responseMessage.data(using: .utf8) ?? Data()
                connection.send(content: responseData, completion: .contentProcessed { error in
                    if let error = error {
                        print("Failed to send response: \(error.localizedDescription)")
                    } else {
                        print("Response sent: \(responseMessage) to \(endpoint)")
                    }
                    connection.cancel()
                })
            case .failed(let error):
                print("Connection failed: \(error)")
                connection.cancel()
            default:
                break
            }
        }
        connection.start(queue: .main)
    }

    func sendHeartbeat() {
        guard let espHost = self.espHost, let espPort = self.espPort else {
            print("ESP host or port is not available. Cannot send HEARTBEAT.")
            return
        }
        let endpoint = NWEndpoint.hostPort(host: espHost, port: espPort)
        let connection = NWConnection(to: endpoint, using: .udp)
        connection.stateUpdateHandler = { newState in
            switch newState {
            case .ready:
                let message = "HEARTBEAT"
                let data = message.data(using: .utf8)
                connection.send(content: data, completion: .contentProcessed { error in
                    if let error = error {
                        print("Failed to send HEARTBEAT: \(error)")
                    } else {
                        print("HEARTBEAT sent to ESP.")
                    }
                    connection.cancel()
                })
            case .failed(let error):
                print("Connection failed: \(error)")
                connection.cancel()
            default:
                break
            }
        }
        connection.start(queue: .main)
    }

    private func startHeartbeatTimer() {
        self.heartbeatTimer?.invalidate()
        self.heartbeatTimer = Timer.scheduledTimer(withTimeInterval: heartbeatInterval, repeats: true) { _ in
            self.checkHeartbeat()
            self.sendHeartbeat()
        }
    }

    private func checkHeartbeat() {
        self.failCount += 1
        if self.failCount >= self.failThreshold {
            print("No heartbeat received from ESP. Disconnected")
            DispatchQueue.main.async {
                self.connected = false
                self.temperature = "--"
                self.humidity = "--"
            }
            self.heartbeatTimer?.invalidate()
            self.failCount = 0
            // Reset ESP host and port
            self.espHost = nil
            self.espPort = nil
        }
    }

    private func parseMessage(_ message: String) {
        print("Received message: \(message)")
        let components = message.split(separator: ",")
        if components.count == 2,
            let temperatureValue = Double(components[0].trimmingCharacters(in: .whitespaces)),
            let humidityValue = Double(components[1].trimmingCharacters(in: .whitespaces)) {
            DispatchQueue.main.async {
                self.temperature = String(format: "%.2f°C", temperatureValue)
                self.humidity = String(format: "%.2f%%", humidityValue)
            }
        } else {
            print("Invalid message format: \(message)")
        }
    }
}
