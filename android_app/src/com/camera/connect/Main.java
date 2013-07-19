package com.camera.connect;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import com.camera.connect.net.* ;

public class Main extends Activity {
    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_PORTRAIT);

        
        Button tbtn, qset, bcast ;
        tbtn = (Button) this.findViewById(R.id.testmodebtn);
        qset = (Button) this.findViewById(R.id.settings);
        bcast = (Button) this.findViewById(R.id.bcast);
        
        
        WebServer ws = new WebServer();
        Thread wthread = new Thread(ws);
        wthread.start();
        


        tbtn.setOnClickListener(new OnClickListener()
        {
        @Override
		public void onClick(View view)
        {
        	
        	Intent myIntent = new Intent(view.getContext(), Testwidget.class);
            startActivityForResult(myIntent, 0);
        }
        });
        
        
        qset.setOnClickListener(new OnClickListener()
        {
        @Override
		public void onClick(View view)
        {
        	
        	Intent myIntent = new Intent(view.getContext(), Qualitysetting.class);
            startActivityForResult(myIntent, 0);
        }
        });
     
        bcast.setOnClickListener(new OnClickListener()
        {
        @Override
		public void onClick(View view)
        {
        	
        	Intent myIntent = new Intent(view.getContext(), Broadcast.class);
            startActivityForResult(myIntent, 0);
        }
        });        
        
        
    }
}