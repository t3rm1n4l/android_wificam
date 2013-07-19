package org.cam.connect;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.SocketException;

public class CamStreamReader {
	
public CamStreamReader() throws IOException
{
	

	DatagramSocket datagramSocket = null;
	try {
		datagramSocket = new DatagramSocket(3000);
	} catch (SocketException e1) {
		// TODO Auto-generated catch block
		e1.printStackTrace();
	}
	FileOutputStream fos = new FileOutputStream("/Users/slynux/devel/rtsp-java/netdat.dat");
	
	System.out.println("About to receive");

	byte[] buffer = new byte[6400];
	DatagramPacket packet = new DatagramPacket(buffer, buffer.length);

	for(;;){
			
		
	
	try {
		datagramSocket.receive(packet);
		
	} catch (IOException e) {
		// TODO Auto-generated catch block
		e.printStackTrace();
	}
	fos.write(packet.getData());
	
	System.out.println("Wrote data to netdata.dat");
	
	
	System.out.println("received packet of size 10 bytes: ");
		}
	
	
	

}


public static void main(String [] args) throws IOException
{
	CamStreamReader c = new CamStreamReader();
	

}
}
