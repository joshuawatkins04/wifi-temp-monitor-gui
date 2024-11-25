import SwiftUI

struct ContentView: View {
  @StateObject private var udpReceiver = UDPReceiver()

  var body: some View {
    VStack(spacing: 20) {
      Text("Temperature:")
        .font (.title)
        .padding(.top, 50)
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
    }
    .padding()
    .onAppear {
      udpReceiver.startListening(port: 12345)
    }
  }
}
