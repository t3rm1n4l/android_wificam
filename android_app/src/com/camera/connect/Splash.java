package com.camera.connect;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.net.UnknownHostException;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.Window;

public class Splash extends Activity {
    /** Called when the activity is first created. */
	
	int showSplashFor = 1500;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        setContentView(R.layout.splash);
    

    	  byte[] b = new byte[4];
    		        							      b[ 0] = new Integer(192).byteValue();
    		        							      b[ 1] = new Integer(168).byteValue();
    		        							      b[ 2] = new Integer(3).byteValue();
    		        							      b[ 3] = new Integer(82).byteValue();

    	
    	
        
        Thread splashScreenTread = new Thread() {
            @Override
            public void run() {
              try {
                int elapsed = 0;
                while(elapsed < showSplashFor) {
                   sleep(100);
                   elapsed += 100;
                }
              } catch(InterruptedException e) {
              } finally {
                finish();
                startActivity(new Intent(Splash.this, Main.class));
                stop();
              }
            }
          };
          splashScreenTread.start();
          
    }
}