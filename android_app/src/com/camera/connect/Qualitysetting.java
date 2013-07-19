package com.camera.connect;

import android.app.Activity;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.widget.ArrayAdapter;
import android.widget.Spinner;
import android.widget.TextView;

public class Qualitysetting extends Activity {
    /** Called when the activity is first created. */
	private String [] resolution, frames;
	public static final String NAME = "CAMCONNECT" ;
	Spinner res,fps;
	TextView ipaddr,port;
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.qualitysetting);
        
        
        resolution = new String[4];
        resolution[0] = "480x320";
        resolution[1] = "352x288";
        resolution[2] = "320x240";
        resolution[3] = "176x144";
        
        frames = new String[4];
        frames[0] = "15";
        frames[1] = "20";
        frames[2] = "10";  
        frames[3] = "5";
        
        res = (Spinner) findViewById(R.id.resolution);
        fps = (Spinner) findViewById(R.id.fps);
        port = (TextView) findViewById(R.id.port);
        ipaddr = (TextView) findViewById(R.id.ipaddr);
        

        ArrayAdapter adapter = new ArrayAdapter(this, android.R.layout.simple_spinner_item, resolution);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        res.setAdapter(adapter);
        adapter = new ArrayAdapter(this, android.R.layout.simple_spinner_item, frames);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        fps.setAdapter(adapter);
        
    	SharedPreferences settings = getSharedPreferences(NAME,0);
    	res.setSelection(settings.getInt("resolution", 0));
    	fps.setSelection(settings.getInt("fps",0));
    	ipaddr.setText(settings.getString("ipaddr", "192.168.0.82"));
    	port.setText(settings.getString("port", "5000"));
    	   	
    	
    }
    
    protected void onDestroy()
    {
    
    	SharedPreferences settings = getSharedPreferences(NAME,0);
        SharedPreferences.Editor editor = settings.edit();
        
    	editor.putInt("resolution", res.getSelectedItemPosition());
    	editor.putInt("fps", fps.getSelectedItemPosition());
    	editor.putString("ipaddr", ipaddr.getText().toString());
    	editor.putString("port", port.getText().toString());
    	
    	
    	editor.commit();

    	super.onDestroy() ;
    }
    
}