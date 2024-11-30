//
//import Network
//
//class UDPReceiver: ObservableObject {
//    @Published var temperature: String = "--"
//    @Published var humidity: String = "--"
//    @Published var connected: Bool = false
//
//    private var listener: NWListener?
//    private var multicastGroup: NWConnectionGroup?
//    private var heartbeatTimer: Timer?
//    private var failCount: Int = 0
//    private let failThreshold: Int = 3
//    private let heartbeatInterval: TimeInterval = 3
//    
//    private let multicastGroupAddress = "224.0.0.1"
//    private let multicastPort: NWEndpoint.Port = 5353
//
//    init(port: UInt16 = 12345) {
//        startListening(port: port)
//        joinMulticastGroup()
//    }
//
//    func startListening(port: UInt16) {
//        do {
//            let params = NWParameters.udp
//            listener = try NWListener(using: params, on: NWEndpoint.Port(rawValue: port)!)
//            listener?.stateUpdateHandler = { state in
//                print("Listening state: \(state)")
//            }
//            listener?.newConnectionHandler = { connection in
//                connection.stateUpdateHandler = { newState in
//                    print("Connection state: \(newState)")
//                    if case .failed(let error) = newState {
//                        print("Connection failed with error: \(error)")
//                    }
//                }
//                connection.start(queue: .main)
//                self.receive(on: connection)
//            }
//            listener?.start(queue: .main)
//        } catch {
//            print("Failed to start listener: \(error)")
//        }
//    }
//
//    func joinMulticastGroup() {
//        guard let multicastHost = NWEndpoint.Host(multicastGroupAddress) else {
//            print("Invalid multicast group address.")
//            return
//        }
//        let params = NWParameters.udp
//        do {
//            let multicastGroupEndpoint = try NWMulticastGroup(host: multicastHost, port: multicastPort)
//            multicastGroup = NWConnectionGroup(with: multicastGroupEndpoint, using: params)
//            multicastGroup?.setReceiveHandler { message, content, isComplete in
//                guard let contentData = content, let messageString = String(data: contentData, encoding: .utf8) else {
//                    print("Received invalid multicast message.")
//                    return
//                }
//                print("Multicast received: \(messageString)")
//                self.handleMulticastMessage(messageString)
//            }
//            multicastGroup?.start(queue: .main)
//        } catch {
//            print("Failed to join multicast group: \(error)")
//        }
//    }
//
//    private func receive(on connection: NWConnection) {
//        connection.receiveMessage { (data, context, isComplete, error) in
//            if let data = data, let message = String(data: data, encoding: .utf8) {
//                print("Received from \(connection.endpoint): \(message)")
//                self.handleMessage(message, from: connection)
//            }
//            if let error = error {
//                print("Error receiving data: \(error)")
//                return
//            }
//            self.receive(on: connection)
//        }
//    }
//
//    private func handleMessage(_ message: String, from connection: NWConnection) {
//        if message == "DISCOVERY_REQUEST" {
//            print("Discovery request received. Sending response...")
//            self.connected = true
//            self.failCount = 0
//            self.startHeartbeatTimer()
//            self.sendResponse("IOS_RESPONSE", to: connection.endpoint)
//        } else if message == "HEARTBEAT" {
//            print("Heartbeat received. Sending ACK...")
//            self.failCount = 0
//            self.sendResponse("HEARTBEAT_ACK", to: connection.endpoint)
//        } else if message.contains(",") {
//            self.parseMessage(message)
//        } else {
//            print("Unknown message received: \(message)")
//        }
//    }
//
//    private func handleMulticastMessage(_ message: String) {
//        if message == "DISCOVERY_REQUEST" {
//            print("Discovery request received via multicast. Sending response...")
//            self.connected = true
//            self.failCount = 0
//            self.startHeartbeatTimer()
//            // Cannot respond to multicast group directly, needs unicast to a specific endpoint.
//        } else if message.contains(",") {
//            self.parseMessage(message)
//        } else {
//            print("Unknown multicast message received: \(message)")
//        }
//    }
//
//    private func sendResponse(_ responseMessage: String, to endpoint: NWEndpoint) {
//        let connection = NWConnection(to: endpoint, using: .udp)
//        connection.stateUpdateHandler = { newState in
//            switch newState {
//            case .ready:
//                let responseData = responseMessage.data(using: .utf8) ?? Data()
//                connection.send(content: responseData, completion: .contentProcessed { error in
//                    if let error = error {
//                        print("Failed to send response: \(error.localizedDescription)")
//                    } else {
//                        print("Response sent: \(responseMessage) to \(endpoint)")
//                    }
//                    connection.cancel()
//                })
//            case .failed(let error):
//                print("Connection failed: \(error)")
//                connection.cancel()
//            default:
//                break
//            }
//        }
//        connection.start(queue: .main)
//    }
//
//    private func startHeartbeatTimer() {
//        self.heartbeatTimer?.invalidate()
//        self.heartbeatTimer = Timer.scheduledTimer(withTimeInterval: heartbeatInterval, repeats: true) { _ in
//            self.checkHeartbeat()
//        }
//    }
//
//    private func checkHeartbeat() {
//        self.failCount += 1
//        if self.failCount >= self.failThreshold {
//            print("No heartbeat received. Disconnected")
//            DispatchQueue.main.async {
//                self.connected = false
//                self.temperature = "--"
//                self.humidity = "--"
//            }
//            self.heartbeatTimer?.invalidate()
//        }
//    }
//
//    private func parseMessage(_ message: String) {
//        let components = message.split(separator: ",")
//        if components.count == 2,
//           let temperatureValue = Double(components[0].trimmingCharacters(in: .whitespaces)),
//           let humidityValue = Double(components[1].trimmingCharacters(in: .whitespaces)) {
//            DispatchQueue.main.async {
//                self.temperature = String(format: "%.2f°C", temperatureValue)
//                self.humidity = String(format: "%.2f%%", humidityValue)
//            }
//        } else {
//            print("Invalid message format: \(message)")
//        }
//    }
//}
