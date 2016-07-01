/*
Arduino Pro Mini 3.3V console port monitor for the Asus RT-AC68U router.

This is a microcontroller running a program that monitors the console port of the Asus RT-AC68U router, to know when the router is ready to begin accepting Linux shell commands. For instance, we can automatically write a passphrase directly into the router's RAM, very early in the boot process, by copying a passphrase text file from the Arduino to the router's /tmp folder, for the purpose of mounting an encrypted disk at startup. Once the router has mounted the encrypted disk, it simply deletes the passphrase file from the /tmp folder, for security purposes.  

Shutdown button - Executes the Linux command "halt" to perform an immediate and orderly shutdown of the router.

Reboot button - Executes the Linux command "reboot" to reboot the router, for convenience.

Compile and flash this sketch to an Arduino Pro Mini 3.3V and then follow the wiring diagram in the file: 
Two-Factor_Pre-Boot_Authentication_for_Asus_Routers.pdf
*/

#include <limits.h>
#include <SoftwareSerial.h>

const char msgBootloaderId[] = "PKID07DC06011801080000000000001A103F01000000";
const unsigned long LEN_BOOTLOADER_ID = sizeof(msgBootloaderId) / sizeof(msgBootloaderId[0]) - 1;

// TODO: uncomment the following line and replace XX-XX-XX-XX-XX-XX with the MAC address of your router; allows us to recognize a specific router
//const char msgRouterId[] = "Device eth0:  hwaddr XX-XX-XX-XX-XX-XX";
// TODO: comment out the following line when the line above is uncommented
const char msgRouterId[] = "Device eth0:  hwaddr ";
const unsigned long LEN_ROUTER_ID = sizeof(msgRouterId) / sizeof(msgRouterId[0]) - 1;

const char msgConsoleReady[] = "Hit ENTER for console...";
const unsigned long LEN_CONSOLE_READY = sizeof(msgConsoleReady) / sizeof(msgConsoleReady[0]) - 1;

// TODO: your passphrase should go on the following line
const char msgPassphrase[] = "REPLACE THIS TEXT WITH YOUR PASSPHRASE";
const unsigned long LEN_BACKUP_KEY = sizeof(msgPassphrase) / sizeof(msgPassphrase[0]) - 1;

const char msgRebootBegin[] = "Rebooting...";
const unsigned long LEN_REBOOT_BEGIN = sizeof(msgRebootBegin) / sizeof(msgRebootBegin[0]) - 1;

const char msgRebootEnd[] = "Restarting system.";
const unsigned long LEN_REBOOT_END = sizeof(msgRebootEnd) / sizeof(msgRebootEnd[0]) - 1;

const char msgShutdownBegin[] = "Shutting down...";
const unsigned long LEN_SHUTDOWN_BEGIN = sizeof(msgShutdownBegin) / sizeof(msgShutdownBegin[0]) - 1;

const char msgShutdownEnd[] = "System halted.";
const unsigned long LEN_SHUTDOWN_END = sizeof(msgShutdownEnd) / sizeof(msgShutdownEnd[0]) - 1;

int incomingByte = 0;
int bootloaderIdMatchPos = -1;
int routerIdMatchPos = -1;
int consoleReadyMatchPos = -1;
int rebootBeginMatchPos = -1;
int rebootEndMatchPos = -1;
int shutdownBeginMatchPos = -1;
int shutdownEndMatchPos = -1;
bool bootloaderIdVerified = false;
bool routerIdVerified = false;
bool consoleReadyToWait = false;
bool consoleReady = false;
bool allowButtons = true;
bool passphraseCopied = false;
bool allowConsoleInput = true;
bool rebootBegin = false;
bool rebootEnd = false;
bool shutdownBegin = false;
bool shutdownEnd = false;
const int ledPin = 13;
const int buttonShutdownPin = 9;
const int buttonRebootPin = 8;
int buttonVal = 0;
bool flashLED = false;
unsigned long flashStartMillis = 0;
unsigned long flashCurrMillis = 0;
unsigned long flashDelayMillis = 0;
unsigned long consoleWaitStartMillis = 0;
unsigned long consoleWaitCurrMillis = 0;
SoftwareSerial mySerial(6, 7); // RX, TX

void setup() 
{
  Serial.begin(115200);
  mySerial.begin(115200);
  
  pinMode(ledPin, OUTPUT);
  pinMode(buttonShutdownPin, INPUT);
  pinMode(buttonRebootPin, INPUT);

  // turn LED off
  digitalWrite(ledPin, LOW); 
  flashLED = false;
  
  bootloaderIdVerified = false;
  routerIdVerified = false;
  consoleReadyToWait = false;
  consoleReady = false;
  allowButtons = true;
  passphraseCopied = false;
  allowConsoleInput = true;
  rebootBegin = false;
  rebootEnd = false;
  shutdownBegin = false;
  shutdownEnd = false;
}

void loop() 
{
  //////////////////////////////////////////////////////////////////////////////////////
  // monitor the console
  
  if (Serial.available()) 
  {
    // monitor the router's console port for keywords
    incomingByte = Serial.read();

    //////////////////////////////////////////////////////////////////////////////////////
    // Detection of the bootloader ID is the starting state.  We always check for the
    // bootloader ID just in case the router crashed and it is rebooted automatically.

    if (!bootloaderIdVerified)
    {
      // try to match string "PKID07DC06011801080000000000001A103F01000000"
      if (incomingByte == msgBootloaderId[++bootloaderIdMatchPos]) // matched a character
      {
        if (bootloaderIdMatchPos == (LEN_BOOTLOADER_ID - 1)) // matched a keyword
        {
          bootloaderIdMatchPos = -1;

          bootloaderIdVerified = true;
          routerIdVerified = false;
          consoleReadyToWait = false;
          consoleReady = false;
          allowButtons = false;
          passphraseCopied = false;
          allowConsoleInput = false;
          rebootBegin = false;
          rebootEnd = false;
          shutdownBegin = false;
          shutdownEnd = false;

          // turn LED off to indicate that the bootloader is starting up
          digitalWrite(ledPin, LOW); // turn LED off
          flashLED = false;
        }
      }
      else
      {
        bootloaderIdMatchPos = -1; // no match
      } 
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // handle reboot and shutdown states for the purpose of disabling button and console
    // input, and blinking the LED

    if (rebootBegin)
    {
      if (!rebootEnd)
      {
        // try to match string "Restarting system."
        if (incomingByte == msgRebootEnd[++rebootEndMatchPos]) // matched a character
        {
          if (rebootEndMatchPos == (LEN_REBOOT_END - 1)) // matched a keyword
          {
            rebootEndMatchPos = -1;
  
            rebootEnd = true;

            // turn LED off to indicate that the router has shutdown and is restarting
            digitalWrite(ledPin, LOW); 
            flashLED = false;
          }
        }
        else
        {
          rebootEndMatchPos = -1; // no match
        }
      }
    }
    else if (shutdownBegin)
    {
      if (!shutdownEnd)
      {
        // try to match string "System halted."
        if (incomingByte == msgShutdownEnd[++shutdownEndMatchPos]) // matched a character
        {
          if (shutdownEndMatchPos == (LEN_SHUTDOWN_END - 1)) // matched a keyword
          {
            shutdownEndMatchPos = -1;
  
            shutdownEnd = true;

            // turn LED off to indicate that the router has shutdown and is restarting
            digitalWrite(ledPin, LOW); 
            flashLED = false;
          }
        }
        else
        {
          shutdownEndMatchPos = -1; // no match
        } 
      }
    }
    else //if (!rebootBegin && !shutdownBegin)
    {
      // try to match string "Rebooting..."
      if (incomingByte == msgRebootBegin[++rebootBeginMatchPos]) // matched a character
      {
        if (rebootBeginMatchPos == (LEN_REBOOT_BEGIN - 1)) // matched a keyword
        {
          rebootBeginMatchPos = -1;
  
          rebootBegin = true;
          allowButtons = false;
          allowConsoleInput = false;
  
          // begin flashing the LED to indicate that the router is rebooting
          flashLED = true;
          flashStartMillis = millis();
          flashDelayMillis = 250;
        }
      }
      else
      {
        rebootBeginMatchPos = -1; // no match
      } 
  
      // try to match string "Shutting down..."
      if (incomingByte == msgShutdownBegin[++shutdownBeginMatchPos]) // matched a character
      {
        if (shutdownBeginMatchPos == (LEN_SHUTDOWN_BEGIN - 1)) // matched a keyword
        {
          shutdownBeginMatchPos = -1;
  
          shutdownBegin = true;
          allowButtons = false;
          allowConsoleInput = false;
  
          // begin flashing the LED to indicate that the router is shutting down
          flashLED = true;
          flashStartMillis = millis();
          flashDelayMillis = 250;
        }
      }
      else
      {
        shutdownBeginMatchPos = -1; // no match
      } 
    }

    //////////////////////////////////////////////////////////////////////////////////////
    // detect other states

    if (!routerIdVerified)
    {
      // try to match string "Device eth0:  hwaddr XX-XX-XX-XX-XX-XX"
      if (incomingByte == msgRouterId[++routerIdMatchPos]) // matched a character
      {
        if (routerIdMatchPos == (LEN_ROUTER_ID - 1)) // matched a keyword
        {
          routerIdMatchPos = -1;

          routerIdVerified = true;
          bootloaderIdVerified = false; // always reset the bootloader ID flag
          
          // begin flashing LED slowly to indicate that the router ID has been verified 
          // and we're waiting for the console to come up
          digitalWrite(ledPin, HIGH); // turn LED on
          flashLED = true;
          flashStartMillis = millis();
          flashDelayMillis = 1000;
        }
      }
      else
      {
        routerIdMatchPos = -1; // no match
      } 
    }
    else if (!consoleReadyToWait)
    {
      if (!consoleReady)
      {
        // try to match string "Hit ENTER for console..."
        if (incomingByte == msgConsoleReady[++consoleReadyMatchPos]) // matched a character
        {
          if (consoleReadyMatchPos == (LEN_CONSOLE_READY - 1)) // matched a keyword
          {
            consoleReadyMatchPos = -1;
  
            consoleReadyToWait = true;

            // begin flashing LED rapidly to indicate that the console is ready 
            // and we're waiting 1 sec. before sending commands to the console
            flashLED = true;
            flashStartMillis = millis();
            flashDelayMillis = 100;
  
            // begin waiting
            consoleWaitStartMillis = millis();        
          }
        }
        else
        {
          consoleReadyMatchPos = -1; // no match
        }
      }
    }

  }

  //////////////////////////////////////////////////////////////////////////////////////
  // check for an operation in progress

  if (consoleReadyToWait)
  {
    consoleWaitCurrMillis = millis();
    if (abs(consoleWaitCurrMillis - consoleWaitStartMillis) > 1000)
    {
      consoleReadyToWait = false;      
      consoleReady = true;
      allowButtons = true;
    }
  }

  if (consoleReady && !passphraseCopied)
  {
//    Serial.println("/bin/cat <<\"EOF\" | /usr/bin/tr -d '\\n' > /tmp/passphrase.key"); // supress newline characters
    Serial.println("/bin/cat <<\"EOF\" > /tmp/passphrase.key");
    Serial.println(msgPassphrase);
    Serial.println("EOF");
    passphraseCopied = true;
    allowConsoleInput = true;
    
    // turn LED on to indicate that the passphrase should now be accessible on the router
    digitalWrite(ledPin, HIGH);
    flashLED = false;
  }

  if (flashLED)
  {
    flashCurrMillis = millis();
    if (abs(flashCurrMillis - flashStartMillis) > flashDelayMillis)
    {
      digitalWrite(ledPin, (digitalRead(ledPin) == HIGH) ? LOW : HIGH); // toggle LED
      flashStartMillis = millis();
    }
  }
  else if (allowButtons)
  {
    // check for shutdown button pressed
    buttonVal = digitalRead(buttonShutdownPin);

    if (buttonVal == LOW)
    {
      Serial.println("halt");
      allowButtons = false;
    }
    else
    {
      // check for reboot button pressed
      buttonVal = digitalRead(buttonRebootPin);
  
      if (buttonVal == LOW)
      {
        Serial.println("reboot");
        allowButtons = false;
      }
    }
  }
  
  //////////////////////////////////////////////////////////////////////////////////////
  // send user command to router

  if (mySerial.available())
  {
    if (allowConsoleInput)
    {
      Serial.write(mySerial.read());
    }
    else
    {
      // discard input
      mySerial.read();
    }
  }
}

