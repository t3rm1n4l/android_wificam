package com.camera.connect ;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

import android.app.Activity;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.res.Configuration;
import android.hardware.Camera;
import android.media.CamcorderProfile;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.SurfaceHolder.Callback;
import android.view.View.OnClickListener;
import android.view.SurfaceView;
import android.widget.Button;

public class Testwidget extends Activity implements SurfaceHolder.Callback
{

		

	SurfaceView cameraView;
	SurfaceHolder surfaceHolder;	
	MediaRecorder recorder;
	
	private static final int FRAME_RATE = 15;

	private static final int CIF_WIDTH = 320;

	private static final int CIF_HEIGHT = 240;
	
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.testwidget);
		
		setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE);
		
		cameraView = (SurfaceView) this.findViewById(R.id.CameraView);
		surfaceHolder = cameraView.getHolder();
		surfaceHolder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS);
		surfaceHolder.addCallback((Callback) this);
		
	

		
		final Button record;
		final Button stop;
		record = (Button) this.findViewById(R.id.record);
		stop = (Button) this.findViewById(R.id.stop);

		record.setOnClickListener(new OnClickListener(){
			
			public void onClick(View view)
	        {
			

			
				recorder.start();
				record.setVisibility(record.GONE);
				stop.setVisibility(stop.VISIBLE);

	        }

		});
		
		
		stop.setOnClickListener(new OnClickListener(){
			
			public void onClick(View view)
	        {			
				recorder.stop();
				record.setVisibility(record.VISIBLE);
				stop.setVisibility(stop.GONE);	
				Intent intent = new Intent(android.content.Intent.ACTION_VIEW);
				
				
				Uri data = Uri.parse(Environment.getExternalStorageDirectory().getPath() + "/Test_Movie.mp4"); 
				
				intent.setDataAndType(data, "video/mp4"); 
				startActivity(intent);
	        }

		});
		
	}
	
	public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
	
	
	} 
	
	public void surfaceCreated(SurfaceHolder holder) {
		recorder = new MediaRecorder();

		recorder.setPreviewDisplay(holder.getSurface());

	
		
		recorder.setVideoSource(MediaRecorder.VideoSource.CAMERA);
		recorder.setOutputFormat(MediaRecorder.OutputFormat.MPEG_4);
		recorder.setVideoEncoder(MediaRecorder.VideoEncoder.H263);
	
		recorder.setVideoFrameRate(FRAME_RATE);
		recorder.setVideoSize(CIF_WIDTH, CIF_HEIGHT);
		recorder.setOutputFile("/sdcard/Test_Movie.mp4");
		
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
	
	
	public void surfaceDestroyed(SurfaceHolder holder) {
		recorder.release();
		

		
		
	}
}



