import SwiftUI
import Network

class UDPReceiver: ObservableObject {
  @Published var temperature: String = "--"
  @Published var humidity: String = "--"
  @Published var connected: Bool = false

  private var listener: NWListener?
  private var heartbeatTimer: Timer?
  private var failCount: Int = 0
  private let failThreshold: Int = 3
  private let heartbeatInterval: TimeInterval = 3

  init(port: UInt16 = 12345) {
    startListening(port: port)
  }

  func startListening(port: UInt16) {
    do {
      listener = try NWListener(using: .udp, on: NWEndpoint.Port(rawValue: port)!)
      listener?.stateUpdateHandler = { state in 
        print("Listening state: \(state)")
      }
      listener?.newConnectionHandler = { connection in
        connection.start(queue: .main)
        self.receive(on: connection)
      }
      listener?.start(queue: .main)
    } catch {
      print("Failed to start listener: \(error)")
    }
  }

  private func receive(on connection: NWConnection) {
    connection.receiveMessage { (data, _, isComplete, error) in
      if let data = data, let message = String(data: data, encoding: .utf8) {
        print("Received: \(message)")
        self.handleMessage(message, from: connection)
      }
      if isComplete {
        self.receive(on: connection)
      } else if error = error {
        print("Error receiving data: \(error)")
      }
    }
  }

  private func handleMessage(_ message: String, from connection: NWConnection) {
    let requestMessage = "DISCOVERY_REQUEST"
    let responseMessage = "IOS_RESPONSE"
    let heartbeatMessage = "HEARTBEAT"
    let heartbeatAckMessage = "HEARTBEAT_ACK"
      
    if message == requestMessage {
      print("Discovery request received. Sending response...")
      self.connected = true
      self.failCount = -
      self.startHeartbeatTimer()
      self.sendResponse(responseMessage, to: connection)
    } else if message == heartbeatMessage {
      print("Heartbeat received. Sending ACK...")
      self.failCount = 0
      self.sendResponse(heartbeatAckMessage, to: connection)
    } else if message.contains(",") {
      self.parseMessage(message)
    } else {
      print("Unknown message received: \(message)")
    }
  }

  private func sendResponse(_ responseMessage: String, to connection: NWConnection) {
    let responseData = responseMessage.data(using: .utf8) ?? Data()
    connection.send(content: responseData, completion: .contentProcessed { error in
      if let error = error {
        print("Failed to send response: \(error.localizedDescription)")
      } else {
        print("Response sent: \(responseMessage)")
      }
    })
  }

  private func startHeartbeatTimer() {
    self.heartbeatTimer?.invalidate()
    self.heartbeatTimer = Timer.scheduleTimer(withTimeInterval: heartbeatInterval, repeat: true) { _ in
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
