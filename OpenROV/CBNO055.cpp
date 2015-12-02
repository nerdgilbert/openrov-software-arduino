#include "AConfig.h"
#if(HAS_BNO055)

#include "NDataManager.h"
#include "CBNO055.h"
#include "CAdaBNO055.h"
#include "CTimer.h"

namespace
{
	CTimer bno055_sample_timer;
	CTimer report_timer;
	CTimer imuTimer;
	CTimer fusionTimer;
	CTimer rawTimer;
	CTimer rawMagTimer;

	bool initalized				= false;
	bool browserPingReceived	= false;

	bool inFusionMode			= false;
	bool waitingToSwitch		= false;

	bool modeSwitchingEnabled	= true;

	CAdaBNO055 bno;

	imu::Vector<3> euler;
	
	imu::Vector<3> rawAccel;
	imu::Vector<3> rawLinearAccel;
	imu::Vector<3> rawGyro;
	imu::Vector<3> rawMag;

	void InitializeSensor()
	{
		// Attempt to initialize. If this fails, we try every 30 seconds in its update loop
		if( !bno.Initialize() )
		{
			Serial.println( "BNO_INIT_STATUS:FAILED;" );
			initalized = false;
		}
		else
		{
			Serial.println( "BNO_INIT_STATUS:SUCCESS;" );


			Serial.print( "BNO055.SW_Revision_ID:" );
			Serial.print( bno.m_softwareVersionMajor, HEX );
			Serial.print( "." );
			Serial.print( bno.m_softwareVersionMinor, HEX );
			Serial.println( ";" );

			Serial.print( "BNO055.bootloader:" );
			Serial.print( bno.m_bootloaderRev );
			Serial.println( ";" );

			initalized = true;
			inFusionMode = true;
		}
	}
}


void CBNO055::Initialize()
{
	// Reset timers
	bno055_sample_timer.Reset();
	report_timer.Reset();
	imuTimer.Reset();
	fusionTimer.Reset();
	rawTimer.Reset();
	rawMagTimer.Reset();
}

void CBNO055::Update( CCommand& commandIn )
{

	if( commandIn.Equals( "ping" ) )
	{
		browserPingReceived = true;
	}

	if( commandIn.Equals( "imumode" ) )
	{
		if( initalized && commandIn.m_arguments[ 0 ] != 0 )
		{
			if( commandIn.m_arguments[ 1 ] == 0 )
			{
				// Turn off override
				modeSwitchingEnabled		= false;
				inFusionMode	= true;
				bno.EnterNDOFMode();

			}

			if( commandIn.m_arguments[ 1 ] == 12 )
			{
				// Override to NDOF
				modeSwitchingEnabled = true;
				bno.EnterNDOFMode();
			}

			if( commandIn.m_arguments[ 1 ] == 8 )
			{
				// Override to IMU mode
				modeSwitchingEnabled = true;
				bno.EnterIMUMode();
			}
		}
		else
		{
			Serial.println( "log:Can't enter override, IMU is not initialized yet!;" );
		}
	}

	
	// Handle raw linear and accel gyro data outputs at 100hz
	if( rawTimer.HasElapsed( 10 ) )
	{
		if( initalized )
		{
			// Raw accelerometer data
			#ifdef BNO_OUTPUT_RAW_ACCEL
		        if( bno.GetVector( CAdaBNO055::VECTOR_ACCELEROMETER, rawAccel ) )
		        {	
		        	Serial.print( "RACC:" );
					Serial.print( rawAccel.x() );
					Serial.print( '|' );
					Serial.print( rawAccel.y() );
					Serial.print( '|' );
					Serial.print( rawAccel.z() );
					Serial.println( ';' );
				}
			#endif
			
			// Raw gyro data
			#ifdef BNO_OUTPUT_RAW_GYRO
		        if( bno.GetVector( CAdaBNO055::VECTOR_GYROSCOPE, rawGyro ) )
		        {	
		        	Serial.print( "RGYR:" );
					Serial.print( rawGyro.x() );
					Serial.print( '|' );
					Serial.print( rawGyro.y() );
					Serial.print( '|' );
					Serial.print( rawGyro.z() );
					Serial.println( ';' );
				}
			#endif
			
			// Linear accel data - Note only available in fusion mode. 0s in IMU mode.
			#ifdef BNO_OUTPUT_RAW_LINEAR_ACCEL
		        if( bno.GetVector( CAdaBNO055::VECTOR_LINEARACCEL, rawLinearAccel ) )
		        {	
		        	Serial.print( "RLACC:" );
					Serial.print( rawLinearAccel.x() );
					Serial.print( '|' );
					Serial.print( rawLinearAccel.y() );
					Serial.print( '|' );
					Serial.print( rawLinearAccel.z() );
					Serial.println( ';' );
				}
			#endif
		}
	}
	
	
	#ifdef BNO_OUTPUT_RAW_MAG		
		// Handle raw mag data outputs at 20hz
		if( rawMagTimer.HasElapsed( 50 ) )
		{
			if( initalized )
			{
				// Get raw mag data - Note this will output 0s if in IMU mode, since the mag is turned off
				if( bno.GetVector( CAdaBNO055::VECTOR_MAGNETOMETER, rawMag ) )
				{	
					Serial.print( "RMAG:" );
					Serial.print( rawMag.x() );
					Serial.print( '|' );
					Serial.print( rawMag.y() );
					Serial.print( '|' );
					Serial.print( rawMag.z() );
					Serial.println( ';' );
				}
				
			}
		}
	#endif
	

	// 1000 / 21
	if( bno055_sample_timer.HasElapsed( 47 ) )
	{
		if( !initalized )
		{
			// Attempt every 10 secs
			if( report_timer.HasElapsed( 10000 ) )
			{
				if( browserPingReceived )
				{
					// Attempt to initialize the chip again
					InitializeSensor();
				}
			}

			return;
		}

		// System status checks
		if( report_timer.HasElapsed( 1000 ) )
		{
			// System calibration
			if( bno.GetCalibration() )
			{
				Serial.print( "BNO055.CALIB_MAG:" );
				Serial.print( bno.m_magCal );
				Serial.println( ';' );
				Serial.print( "BNO055.CALIB_ACC:" );
				Serial.print( bno.m_accelCal );
				Serial.println( ';' );
				Serial.print( "BNO055.CALIB_GYR:" );
				Serial.print( bno.m_gyroCal );
				Serial.println( ';' );
				Serial.print( "BNO055.CALIB_SYS:" );
				Serial.print( bno.m_systemCal );
				Serial.println( ';' );

				// Get offsets
				bno.GetGyroOffsets();
				bno.GetAccelerometerOffsets();
				bno.GetMagnetometerOffsets();
			}
			else
			{
				Serial.print( "BNO055.CALIB_MAG:" );
				Serial.print( "N/A" );
				Serial.println( ';' );
				Serial.print( "BNO055.CALIB_ACC:" );
				Serial.print( "N/A" );
				Serial.println( ';' );
				Serial.print( "BNO055.CALIB_GYR:" );
				Serial.print( "N/A" );
				Serial.println( ';' );
				Serial.print( "BNO055.CALIB_SYS:" );
				Serial.print( "N/A" );
				Serial.println( ';' );
			}

			// Operating mode
			if( bno.GetOperatingMode() )
			{
				Serial.print( "BNO055.MODE:" );
				Serial.print( bno.m_operatingMode );
				Serial.println( ';' );
			}
			else
			{
				Serial.print( "BNO055.MODE:" );
				Serial.print( "N/A" );
				Serial.println( ';' );
			}

			// System status
			if( bno.GetSystemStatus() )
			{
				Serial.print( "BNO055_STATUS:" );
				Serial.print( bno.m_systemStatus, HEX );
				Serial.println( ";" );
			}
			else
			{
				Serial.print( "BNO055_STATUS:" );
				Serial.print( "N/A" );
				Serial.println( ";" );
			}

			// System Error
			if( bno.GetSystemError() )
			{
				Serial.print( "BNO055_ERROR_FLAG:" );
				Serial.print( bno.m_systemError );
				Serial.println( ";" );
			}
			else
			{
				Serial.print( "BNO055_ERROR_FLAG:" );
				Serial.print( "N/A" );
				Serial.println( ";" );
			}

		}

		// Get orientation data
        if( bno.GetVector( CAdaBNO055::VECTOR_EULER, euler ) )
        {			
            // Throw out exactly zero heading values that are all zeroes - necessary when switching modes
            if( euler.x() != 0.0f  )
            {
                // These may need adjusting
                
                NDataManager::m_navData.YAW		= euler.x();
                NDataManager::m_navData.HDGD	= euler.x();
            }
			
			NDataManager::m_navData.PITC	= euler.z();
			NDataManager::m_navData.ROLL	= -euler.y();
        }

	
		if( !modeSwitchingEnabled )
		{
			// Temp - Do nothing!
		}
		else
		{
			// If we're in fusion mode, check to see if we have a good mag and system calibration
			if( inFusionMode )
			{
				// If motors ever come on during calibration, drop to IMU mode
				if( NDataManager::m_thrusterData.MotorsActive )
				{
					// Switch to gyro mode
					bno.EnterIMUMode();
					inFusionMode = false;

					imuTimer.Reset();
				}

				// Try to stay in fusion mode until some kind of calibration is achieved, and then for at least three seconds to get a better calibration
				//if( bno.m_magCal != 0 && bno.m_systemCal != 0 )
				//{
				//	if( fusionTimer.HasElapsed( 3000 ) )
				//	{
				//		// Switch to gyro mode
				//		bno.EnterIMUMode();
				//		inFusionMode = false;

				//		// Reset the timer
				//		imuTimer.Reset();
				//	}
				//}
			}
			else
			{
				if( waitingToSwitch )
				{
					// Make sure motors aren't active
					if( NDataManager::m_thrusterData.MotorsActive == false )
					{
						// Switch modes
						bno.EnterNDOFMode();
						fusionTimer.Reset();
						inFusionMode	= true;
						waitingToSwitch = false;
					}
				}
				else
				{
					// Check to see if proper amount of time has elapsed before switching back to fusion mode
					if( imuTimer.HasElapsed( 5000 ) )
					{
						// Make sure motors aren't active
						if( NDataManager::m_thrusterData.MotorsActive == false )
						{
							// Switch modes
							bno.EnterNDOFMode();
							fusionTimer.Reset();
							inFusionMode	= true;
							waitingToSwitch = false;
						}
						else
						{
							// Not ready to switch because motors are on
							waitingToSwitch = true;
						}
					}
				}
			}
		}
	}
}
#endif
