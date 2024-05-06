// C++ code
//
void setup()
{
  pinMode(12, INPUT);
  pinMode(11, OUTPUT);
  
}

void loop()
{
  if (digitalRead(12)){
  digitalWrite(11, HIGH);
 
  }
  else{
      digitalWrite(11, LOW);
  }
}