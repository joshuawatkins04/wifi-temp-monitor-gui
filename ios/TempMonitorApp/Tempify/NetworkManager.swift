import SwiftUI
import Network

class UDPReceiver: ObservableObject {
  @Published var temperature: String = "--"
  @Published var humidity: String = "--"
  @Published var connected: Bool = false
  @Published var packetsReceived: Int = 0
  @Published var packetsSent: Int = 0

  private var listener: NWListener?
  private var heartbeatTimer: Timer?
  private var connectivityTimer: Timer?
  private var lastPacketReceived: Date = Date()
  private let timeoutInterval: TimeInterval = 10
  private var failCount: Int = 0
  private let failThreshold: Int = 3
  private let heartbeatInterval: TimeInterval = 3
  
  private var serverEndpoint: NWEndpoint?
  private var serverHost: NWEndpoint.Host?
  private var serverPort: NWEndpoint.Port?

  init(port: UInt16 = 12345) {
    startListening(port: port)
    startConnectivityTimer()
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
  
  private func startConnectivityTimer() {
    connectivityTimer?.invalidate()
    connectivityTimer = Timer.scheduledTimer(withTimeInterval: 1.0, repeats: true) { [weak self] _ in
      self?.checkConnectivity()
    }
  }
  
  private func checkConnectivity() {
    let timeSinceLastPacket = Date().timeIntervalSince(lastPacketReceived)
    
    if timeSinceLastPacket > timeoutInterval {
      if self.connected {
        print("No packets received within \(timeoutInterval) seconds. Disconnecting...")
        DispatchQueue.main.async {
          self.connected = false
          self.temperature = "--"
          self.humidity = "--"
        }
      }
    }
    lastPacketReceived = Date() + timeoutInterval
    print("Timer reset")
  }

  private func receive(on connection: NWConnection) {
    connection.receiveMessage { (data, context, isComplete, error) in
      if let data = data, let message = String(data: data, encoding: .utf8) {
        self.lastPacketReceived = Date()
        self.packetsReceived += 1
        self.connected = true
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

  private func handleMessage(_ message: String, from connection: NWConnection) {
    self.serverEndpoint = connection.endpoint
    
    if case let NWEndpoint.hostPort(host, port) = connection.endpoint {
      self.serverHost = host
      self.serverPort = port
    }
    
    let requestMessage = "DISCOVERY_REQUEST"
    let responseMessage = "IOS_RESPONSE"
    let heartbeatMessage = "HEARTBEAT"
    let heartbeatAckMessage = "HEARTBEAT_ACK"
      
    if message == requestMessage {
      print("Discovery request received. Sending response...")
      self.connected = true
      self.failCount = 0
      self.startHeartbeatTimer()
      self.sendResponse(responseMessage, to: connection.endpoint)
    } else if message == heartbeatMessage {
      print("Heartbeat received. Sending ACK...")
      self.failCount = 0
      self.sendResponse(heartbeatAckMessage, to: connection.endpoint)
    } else if message.contains(",") {
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
            self.packetsSent += 1
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

  func sendEspConnectMessage() {
    let targetIP = "192.168.0.171"
    let targetPort: UInt16 = 12345
    var responseMessage = ""
    
    if connected {
      responseMessage = "DATA_REQ"
    } else {
      responseMessage = "IOS_CONNECT"
    }
    
    let host = NWEndpoint.Host(targetIP)

    print("Sending response to \(targetIP): \(responseMessage)")
    
    guard let port = NWEndpoint.Port(rawValue: targetPort) else {
      print("Invalid port number: \(targetPort)")
      return
    }
    
    let connection = NWConnection(host: host, port: port, using: .udp)
    
    connection.stateUpdateHandler = { newState in
      switch newState {
      case .ready:
        let responseData = responseMessage.data(using: .utf8) ?? Data()
        connection.send(content: responseData, completion: .contentProcessed { error in
          if let error = error {
            print("Failed to send response: \(error.localizedDescription)")
          } else {
            self.packetsSent += 1
            print("Response sent: \(responseMessage) to \(targetIP):\(targetPort)")
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
  
  // func sendReconnectMessage() {
  //   guard let serverHost = self.serverHost, let serverPort = self.serverPort else {
  //     print("Server host or port is not available. Cannot send RECONNECT message.")
  //     return
  //   }
  //   let endpoint = NWEndpoint.hostPort(host: serverHost, port: serverPort)
  //   let connection = NWConnection(to: endpoint, using: .udp)
    
  //   connection.stateUpdateHandler = { newState in
  //     switch newState {
  //     case .ready:
  //       let message = "RECONNECT"
  //       let data = message.data(using: .utf8)
  //       connection.send(content: data, completion: .contentProcessed { error in
  //         if let error = error {
  //           print("Failed to send RECONNECT message: \(error)")
  //         } else {
  //           print("RECONNECT message sent.")
  //         }
  //       })
  //     case .failed(let error):
  //       print("Connection failed: \(error)")
  //       connection.cancel()
  //     default:
  //       break
  //     }
  //   }
  //   connection.start(queue: .main)
  // }

  private func startHeartbeatTimer() {
    self.heartbeatTimer?.invalidate()
    self.heartbeatTimer = Timer.scheduledTimer(withTimeInterval: heartbeatInterval, repeats: true) { _ in
      self.checkHeartbeat()
    }
  }

  private func checkHeartbeat() {
    self.failCount += 1
    if self.failCount >= self.failThreshold {
      print("No heartbeat received. Disconnected")
      DispatchQueue.main.async {
        self.connected = false
        self.temperature = "--"
        self.humidity = "--"
      }
      self.heartbeatTimer?.invalidate()
    }
  }

  private func parseMessage(_ message: String) {
    print("Received: message: \(message)")
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
