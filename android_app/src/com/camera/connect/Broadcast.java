package com.camera.connect ;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.UnknownHostException;

import com.camera.connect.net.RtpPacket;
import com.camera.connect.net.RtpSocket;

import android.R.bool;
import android.app.Activity;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.graphics.drawable.Drawable.Callback;
import android.media.MediaRecorder;
import android.net.LocalServerSocket;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.os.Bundle;
import android.os.Handler;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.util.Log;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.Toast;


public class Broadcast extends Activity implements SurfaceHolder.Callback
{

		
	LocalSocket receiver,sender;
	LocalServerSocket lss;
	SurfaceView cameraView;
	SurfaceHolder surfaceHolder;	
	MediaRecorder recorder;
	ParcelFileDescriptor pfd;
	
	private static final int FRAME_RATE = 40;
	private static final int CIF_WIDTH = 640;
	private static final int CIF_HEIGHT = 480;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.broadcast);
		
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
		cameraView = (SurfaceView) this.findViewById(R.id.CameraView);
		surfaceHolder = cameraView.getHolder();
		surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		surfaceHolder.addCallback((android.view.SurfaceHolder.Callback) this);
	}
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
	    MenuInflater inflater = getMenuInflater();
	    inflater.inflate(R.menu.broadcast, menu);
	    return true;
	}
	
	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
	    switch (item.getItemId()) {
	        case R.id.stream:     Toast.makeText(this, "Waiting for connect", Toast.LENGTH_LONG).show();
			
			
			//receiver = new LocalSocket();
			/*try {
				lss = new LocalServerSocket("camsocket");
				receiver.connect(new LocalSocketAddress("camsocket"));
				receiver.setReceiveBufferSize(500000);
				receiver.setSendBufferSize(500000);
				sender = lss.accept();
				sender.setReceiveBufferSize(500000);
				sender.setSendBufferSize(500000);
				
				
				
		


				

			} catch (Exception e) {
				// TODO Auto-generated catch block
				Log.v("CAMCONNECT", "Recorder start() error");
				e.printStackTrace();
			}
			
			*/
	        					// recorder.reset();
	        					  Log.v("CAMCONNECT", "Set recorder");
	        					 //setRecorder(true);
	        					  Log.v("CAMCONNECT", "About to recorder start");

	        					 //recorder.start();
	        					  
	        					  
	        					  final Handler mHandler = new Handler();
	        					  
	        					  final Runnable errconnect = new Runnable() {
	        					      public void run() {
											 Toast.makeText(Broadcast.this, "Cannot connect. Please check Wifi connection.", Toast.LENGTH_LONG).show();
	        					      }
	        					  };
	        					  
	        					  final Runnable connected = new Runnable() {
	        					      public void run() {
											 Toast.makeText(Broadcast.this, "Started streaming...", Toast.LENGTH_LONG).show();
	        					      }
	        					  };
	    
		
	        				      Log.v("CAMCONNECT", "Recording started!");

	        					  Thread t = new Thread()
	        					  {
	        						  Socket csock ;
	        						  public void run()
	        						  {
	        	    					  
	        							
	        							  String [] iparray = null;
	        							  SharedPreferences settings = getSharedPreferences("CAMCONNECT",0);
	        							  String ipaddr = settings.getString("ipaddr", "192.168.0.82");
	        							  String port = settings.getString("port", "5000");
	        							  
	        							  Log.v("CAMCONNECT","Before split = ");
	        							  iparray = ipaddr.split("\\.");


	        							  byte [] b = new byte[4];
	        							  b[0] = new Integer(Integer.parseInt(iparray[0])).byteValue();
	        							  b[1] = new Integer(Integer.parseInt(iparray[1])).byteValue();
	        							  b[2] = new Integer(Integer.parseInt(iparray[2])).byteValue();
	        							  b[3] = new Integer(Integer.parseInt(iparray[3])).byteValue();
	  
	        							  try {
											csock = new Socket(InetAddress.getByAddress(b), Integer.parseInt(port));
											mHandler.post(connected);

										} catch (Exception e2) {

											Log.v("CAMCONNECT","Socket creation error");
											mHandler.post(errconnect);
											return;
											
											
										}
	        							  
	        							  byte []buff = new byte[1024];
	        							  byte []ignore = new byte[25];
	        						
        							      
        							      InputStream fis = null;
        							      OutputStream outs = null;
        								  File camstream = new File("/sdcard/Test_Movie.mp4");
        							      FileInputStream fin = null;
        							      
										try {
											fin = new FileInputStream(camstream);
										} catch (FileNotFoundException e2) {
											// TODO Auto-generated catch block
											e2.printStackTrace();
										}
        									
        							      BufferedInputStream bis = new BufferedInputStream(fin);
        							      
        							      try {
	        							    outs = csock.getOutputStream();

									
										} catch (IOException e1) {
											// TODO Auto-generated catch block
											Log.v("CAMCONNECT","Socket creation error");
										}
										
								
        							      int read = 0;
        							      try {
        							    	bis.read(ignore);
       							      
        							      while (true){
        							    	
        							    	  read = bis.read(buff);

        							    	  if (read==-1){
        							    		  Log.v("CAMCONNECT", "Not avail");
        							    		 sleep(1000);
        							    		  continue;
        							    	  }
        							    	  else{
            							    	  Log.v("CAMCONNECT", "Sending packet with size="+read);

            							    	  outs.write(buff,0,read);
        							    	  
        							    	  }
					
										
        							      }
										
										//csock.close();
        							      
        									} catch (Exception e) {
    											// TODO Auto-generated catch block
        										
          							    	  Log.v("CAMCONNECT", "ERROR");

    										}
	        							  /*
	        							  int frame_size = 1400;
	        							  byte[] buffer = new byte[frame_size + 14];
	        							  RtpPacket rtp_packet = new RtpPacket(buffer,0);
	        							  RtpSocket rtp_socket = null;
	        							  int seqn = 0;
	        							  int num, number = 0, src, dest;
	        							  InputStream fis = null;
	        							  try{
	        								  
	        								  byte[] b = new byte[4];
	        							      b[ 0] = new Integer(192).byteValue();
	        							      b[ 1] = new Integer(168).byteValue();
	        							      b[ 2] = new Integer(3).byteValue();
	        							      b[ 3] = new Integer(82).byteValue();
	        								  rtp_socket = new RtpSocket(new DatagramSocket(4000),InetAddress.getByAddress(b),3000);

	        								 
	        							  }
	        							  catch (Exception e)
	        							  {
	        								  e.printStackTrace();
	        							  }
	        							  
	        							
	        							  
	        							  while (fis == null){
	        								  try{
	        									  fis = receiver.getInputStream();
	        								  }
	        								  catch (Exception e){
	        									  try{
	        										  sleep(1000);
	        									  }
	        									  catch (InterruptedException e2)
	        									  {
	        										  rtp_socket.getDatagramSocket().close();
	        										  return ;
	        									  }
	        								  }
	        							  }
	        							  rtp_packet.setPayloadType(103);
	        							  while (true)
	        							  {
	        								  
	        									Log.v("CAMCONNECT", "Inside thread:"+String.valueOf(seqn));

	        								  num = -1;
	        								  try{
	        									  num = fis.read(buffer, 14+number, frame_size-number);
	        								  }
	        								  catch (IOException e)
	        								  {
	        									  e.printStackTrace();
	        								  }
	        								  if (num < 0){
	        									  try{
	        										  sleep(100);
	        									  } catch (InterruptedException e)
	        									  {
	        										  break;
	        									  }
	        									  continue;
	        								  }
	        									number += num;
	        		    						
	        		    						for (num = 14+number-2; num > 14; num--)
	        		    							if (buffer[num] == 0 && buffer[num+1] == 0) break;
	        		    						if (num == 14) {
	        		    							num = 0;
	        		    							rtp_packet.setMarker(false);
	        		    						} else {	
	        		    							num = 14+number - num;
	        		    							rtp_packet.setMarker(true);
	        		    						}
	        		    						
	        		    			 			rtp_packet.setSequenceNumber(seqn++);
	        		    			 			rtp_packet.setTimestamp(SystemClock.elapsedRealtime()*90);
	        		    			 			rtp_packet.setPayloadLength(number-num+2);
	        								  
	        								  Log.v("CAMCONNECT", "SENDING pkt");
	        								  
	        								  try {
	        							
        									  rtp_socket.send(rtp_packet);
	        								  
	        								  }
	        								  catch (IOException e)
	        								  {
	        									  e.printStackTrace();
	        								  }
		        							//  rtp_socket.getDatagramSocket().close();
	        						 			if (num > 0) {
	        		    				 			num -= 2;
	        		    				 			dest = 14;
	        		    				 			src = 14+number - num;
	        		    				 			number = num;
	        		    				 			while (num-- > 0)
	        		    				 				buffer[dest++] = buffer[src++];
	        		    							buffer[12] = 4;
	        		    			 			} else {
	        		    			 				number = 0;
	        		    							buffer[12] = 0;
	        		    			 			}

	        							  }
	        							  rtp_socket.getDatagramSocket().close();
	        							  
	    	        	*/    							  
	        						  }
	        						  

	        						  
	        					  };
	        					  
	        					  t.start();

	        					  
	        					  
	        					  
	        
	                            break;
	        case R.id.stop_stream:     Toast.makeText(this, "Stopped streaming", Toast.LENGTH_LONG).show();
	        						  recorder.stop();
			
	        						  setRecorder(true);
	        							
	                            break;
	  
	    }
	    return true;
	}

	public void setRecorder(boolean Nulldev)
	{
				   
		  	recorder.setPreviewDisplay(surfaceHolder.getSurface());
			
			recorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
			recorder.setOutputFormat(MediaRecorder.OutputFormat.THREE_GPP);
			recorder.setVideoEncoder(MediaRecorder.VideoEncoder.H263);
		
		   
		    SharedPreferences settings = getSharedPreferences("CAMCONNET",0);
		        
		    if (settings.getInt("fps",0)==0)
		    	recorder.setVideoFrameRate(15);
		    
		    if (settings.getInt("fps",0)==1)
		    	recorder.setVideoFrameRate(20);	
		    
		    if (settings.getInt("fps",0)==2)
		    	recorder.setVideoFrameRate(10);
		    
		    if (settings.getInt("fps",0)==3)
		    	recorder.setVideoFrameRate(5);	
			
		    if (settings.getInt("resolution", 0)==0)
		    	recorder.setVideoSize(480, 320);

		    if (settings.getInt("resolution", 0)==1)
		    	recorder.setVideoSize(352, 288);
		    
		    if (settings.getInt("resolution", 0)==2)
		    	recorder.setVideoSize(320, 240);
		    
		    if (settings.getInt("resolution", 0)==3)
		    	recorder.setVideoSize(176, 144);
		    
		    
			if (Nulldev)
				//recorder.setOutputFile("/dev/null") ;
				recorder.setOutputFile("/sdcard/Test_Movie.mp4") ;

			else
			{
				//recorder.setOutputFile("/sdcard/Test_Movie.mp4") ;

				//recorder.setOutputFile(sender.getFileDescriptor());
			}
			
			try {
				recorder.prepare();
			} catch (IllegalStateException e) {
				// TODO Auto-generated catch block
				Log.v("CAMCONNECT","recorder.prepare() failed()") ;
			} catch (IOException e) {
				// TODO Auto-generated catch block
				Log.v("CAMCONNECT","Camera.prepare() failed()") ;
			}		

	}
	
	
	@Override
	public void surfaceChanged(SurfaceHolder holder, int format, int width,
			int height) {
		// TODO Auto-generated method stub
		
	}

	@Override
	public void surfaceCreated(SurfaceHolder holder) {
		recorder = new MediaRecorder();

		setRecorder(true);
		recorder.start();
				
	}

	@Override
	public void surfaceDestroyed(SurfaceHolder holder) {
		recorder.release();
		
	}
	
	

}



