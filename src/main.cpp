// Firmware-Version 2.3
// Befehle:
// 111 -> Sperren der Klappe -> Ersetzen durch "LOCK"
// 222 -> Entsperren der Klappe -> Ersetzen durch "UNLOCK"
// 333 -> Status abfragen -> Ersetzen durch "GET_STATE"

#include <Arduino.h>
#include <SerialCommands.h>

#define UNDEFINED 0
#define LOCKED 1
#define UNLOCKED 2
#define LOCKING 3
#define OPEN 4

//Create a 32 bytes static buffer to be used exclusive by SerialCommands object.
//The size should accomodate command token, arguments, termination sequence and string delimeter \0 char.
char serial_command_buffer_[32];

//Creates SerialCommands object attached to Serial
//working buffer = serial_command_buffer_
//command delimeter: Cr & Lf
//argument delimeter: SPACE
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r\n", " ");

const float version_nr = 2.3;
const int pin_valve = A3;
const int pin_switch = 2;

int currentstate = UNDEFINED;
int setstate = 2;
int serial_input = 0;
int triggering_state = 1;

//expects one argument value (relative position in millimeter)
void cmd_command(SerialCommands *sender)
{
  //Note: Every call to Next moves the pointer to next parameter
  char *cmd_number = sender->Next();
  if (cmd_number == NULL)
  {
    sender->GetSerial()->println("ERROR NO_COMMAND");
    return;
  }

  serial_input = atoi(cmd_number);
  sender->GetSerial()->print("COMMAND=");
  sender->GetSerial()->println(serial_input);
}

void cmd_unrecognized(SerialCommands *sender, const char *cmd)
{
  sender->GetSerial()->print("ERROR: Unrecognized command [");
  sender->GetSerial()->print(cmd);
  sender->GetSerial()->println("]");
}

/* Commands */
SerialCommand cmd_command_("COMMAND", cmd_command);

void print_state()
{
  switch (currentstate)
  {
  case UNDEFINED:
    Serial.println("UNDEFINED");
    break;
  case LOCKED:
    Serial.println("LOCKED");
    break;
  case UNLOCKED:
    Serial.println("UNLOCKED");
    break;
  case LOCKING:
    Serial.println("LOCKING");
    break;
  case OPEN:
    Serial.println("OPEN");
    break;
  default:
    break;
  }
}

void set_state(int state)
{
  if (currentstate != state)
  {
    currentstate = state;
    print_state();
  }
}

void setup()
{
  pinMode(pin_valve, OUTPUT);
  pinMode(pin_switch, INPUT);

  Serial.begin(9600);
  while (!Serial)
  {
    delay(1);
  }

  Serial.println("starting lock");
  Serial.print("Firmware Version: ");
  Serial.println(version_nr, 2);

  serial_commands_.SetDefaultHandler(&cmd_unrecognized);
  serial_commands_.AddCommand(&cmd_command_);
}

void loop()
{
  //zielzustaende setzen
  if (Serial.available())
  {
    serial_commands_.ReadSerial();

    if (serial_input == 111)
    {
      set_state(LOCKING);
      setstate = 1;
      triggering_state = 1;
      serial_input = 999; //idle zustand
    }
    else if (serial_input == 222)
    {
      setstate = 2;
      triggering_state = 1;
      serial_input = 999; //idle zustand
    }
    else if (serial_input == 333)
    {
      print_state();

      serial_input = 999; //idle zustand
    }
    else
    {
    }
  }

  //zustaende einnehmen
  if (setstate == 1 && triggering_state == 1 && digitalRead(pin_switch) == LOW)
  {
    //sperren, wenn klappe geschlossen und schalter ausgeloest ist
    digitalWrite(pin_valve, HIGH);
    triggering_state = 0;
    set_state(LOCKED);
  }
  else if (setstate == 2 && triggering_state == 1)
  {
    //entsperren
    digitalWrite(pin_valve, LOW);
    triggering_state = 0;
    set_state(UNLOCKED);
  }

  if (currentstate == UNLOCKED && digitalRead(pin_switch) == HIGH)
  {
    set_state(OPEN);
  }
  else if (currentstate == OPEN && digitalRead(pin_switch) == LOW)
  {
    set_state(UNLOCKED);
  }
}
