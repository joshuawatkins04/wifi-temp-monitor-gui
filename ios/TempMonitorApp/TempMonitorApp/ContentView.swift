import SwiftUI

struct ContentView: View {
  @StateObject private var udpReceiver = UDPReceiver()

  var body: some View {
    VStack(spacing: 20) {
      Text("Connection Status: \(udpReceiver.connected ? "Connected" : "Disconnected")")
        .font(.headline)
        .padding(.top, 20)

      Text("Temperature:")
        .font (.title)
        .padding(.top, 20)
      Text(udpReceiver.temperature)
        .font(.system(size: 48))
        .bold()
        .foregroundColor(.blue)
      
      Text("Humidity")
        .font(.title)
      Text(udpReceiver.humidity)
        .font(.system(size: 48))
        .bold()
        .foregroundColor(.green)

      Spacer()

      Button(action: {
      udpReceiver.startMulticastListening() // Start multicast discovery to connect devices
      }) {
        Text("Start Discovery")
          .padding()
          .background(Color.blue)
          .foregroundColor(.white)
          .cornerRadius(10)
      }
      .padding(.bottom, 20)

      Button(action: {
        udpReceiver.sendReconnectMessage() // Send reconnect message
      }) {
        Text("Send RECONNECT")
          .padding()
          .background(Color.green)
          .foregroundColor(.white)
          .cornerRadius(10)
      }
      .padding(.bottom, 20)
    }
    .padding()
    .onAppear {
      udpReceiver.startListening(port: 12345)
      udpReceiver.startMulticastListening()
    }
  }
}
