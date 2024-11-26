import SwiftUI
import Network

class UDPReceiver: ObservableObject {
  @Published var temperature: String = "--"
  @Published var humidity: String = "--"

  private var listener: NWListener?
  private var connection: NWConnection?

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
        DispatchQueue.global().asyncAfter(deadline: .now() + 1) {
          connection.cancel()
        }
      }
    }
  }

  private func handleMessage(_ message: String, from connection: NWConnection) {
    let requestMessage = "DISCOVERY_REQUEST"
    let responseMessage = "IOS_RESPONSE"
    let targetPort: NWEndpoint.Port = 12345
      
    if message == requestMessage {
      print("Discovery request received. Sending response...")
      
      if case let .hostPort(host, _) = connection.endpoint {
        print("Extracted host: \(host)")
        let newConnection = NWConnection(host: host, port: targetPort, using: .udp)
        newConnection.start(queue: .main)
        
        let responseData = responseMessage.data(using: .utf8) ?? Data()
        newConnection.send(content: responseData, completion: .contentProcessed { error in
          if let error = error {
            print("Failed to send response: \(error.localizedDescription)")
          } else {
            print("Response sent: \(responseMessage) to \(host):\(targetPort)")
          }
          newConnection.cancel()
        })
      } else {
        print("Failed to extract host from connection endpoint.")
      }
    } else {
      self.parseMessage(message)
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
