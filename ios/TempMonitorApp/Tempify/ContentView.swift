import SwiftUI

struct ContentView: View {
  @StateObject private var udpReceiver = UDPReceiver()
  @State private var isHovered = false
  @State private var isMenuVisible = false

  var body: some View {
    ZStack {
      Color.white.edgesIgnoringSafeArea(.all)
      
      if isMenuVisible {
        VStack(spacing: 20) {
          Spacer()
          Text("Temperature:")
            .font (.title)
            .padding(.top, 20)
          Text(udpReceiver.temperature)
            .font(.system(size: 48))
            .bold()
            .foregroundColor(.blue)
          
          Text("Humidity:")
            .font(.title)
          Text(udpReceiver.humidity)
            .font(.system(size: 48))
            .bold()
            .foregroundColor(.green)
          
          Spacer()
          
          Button(action: {
            udpReceiver.sendEspConnectMessage() // Send reconnect message
          }) {
            Text("Get Data")
              .padding(10)
              .font(.system(size: 15))
              .background(isHovered ? Color.gray : Color.white)
              .foregroundColor(.black)
              .cornerRadius(10)
              .shadow(color: .black.opacity(0.2), radius: 6, x: 0, y: 0)
//              .padding(.horizontal, 30)
              .scaleEffect(isHovered ? 1.1 : 1.0)
              .animation(.easeInOut(duration: 0.2), value: isHovered)
          }
          .onHover { hovering in
            isHovered = hovering
          }
        }
        .padding()
        .transition(.opacity)
        .animation(.easeInOut, value: isMenuVisible)
      } else {
        VStack(spacing: 20) {
        Text("Connection Status: \(udpReceiver.connected ? "Connected" : "Disconnected")")
            .font(.title2)
          .padding(.top, 20)
        Text("Packets Sent: \(udpReceiver.packetsSent)")
          .font(.title2)
          Text("Packets Received: \(udpReceiver.packetsReceived)")
            .font(.title2)
          Button(action: {
            udpReceiver.sendEspConnectMessage() // Send reconnect message
          }) {
            Text("Connect")
              .padding()
              .background(isHovered ? Color.gray : Color.white)
              .foregroundColor(.black)
              .cornerRadius(10)
              .shadow(color: .black.opacity(0.2), radius: 6, x: 0, y: 0)
              .padding(.horizontal, 20)
              .scaleEffect(isHovered ? 1.1 : 1.0)
              .animation(.easeInOut(duration: 0.2), value: isHovered)
          }
          .onHover { hovering in
            isHovered = hovering
          }
      }
        .transition(.opacity)
        .animation(.easeInOut, value: isMenuVisible)
    }
      
      VStack {
        HStack {
          Spacer()
          Button(action: {
            withAnimation {
              isMenuVisible.toggle()
            }
          }) {
            Image(systemName: isMenuVisible ? "xmark.circle.fill" : "ellipsis.circle.fill")
              .resizable()
              .frame(width: 30, height: 30)
              .foregroundColor(.white)
              .background(Circle().fill(Color.black))
              .shadow(color: .black.opacity(0.2), radius: 6, x: 0, y: 0)
              .padding()
          }
        }
        Spacer()
      }
    }
    .onAppear {
      udpReceiver.startListening(port: 12345)
      // udpReceiver.startMulticastListening()
    }
  }
}
