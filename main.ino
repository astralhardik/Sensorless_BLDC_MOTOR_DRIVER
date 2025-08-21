#define SPEED_UP        A0          // Button connected to analog pin A0; pressing this increases BLDC motor speed
#define SPEED_DOWN      A1          // Button connected to analog pin A1; pressing this decreases BLDC motor speed
#define PWM_MAX_DUTY    255         // Maximum allowed PWM duty cycle for motor speed
#define PWM_MIN_DUTY    50          // Minimum allowed PWM duty cycle for motor speed (prevents stalling)
#define PWM_START_DUTY  100         // Default starting PWM duty cycle when motor starts

byte bldc_step = 0, motor_speed;    // Sequence step for BLDC commutation (0-5), current motor speed
unsigned int i;

void setup() {
  // --- Configure MCU Pins for BLDC motor ---
  DDRD  |= 0x38;         // Set pins D3, D4, D5 as outputs (D3: Low B, D4: Low A, D5: unused/buzzer)
  PORTD = 0x00;          // Set all PORTD output pins to LOW initially
  DDRB  |= 0x0E;         // Set pins D9, D10, D11 as outputs (D9: High A, D10: High B, D11: High C)
  PORTB = 0x31;          // Set PORTB outputs D9, D10, D11 to LOW; D13 high (if used for status LED)
  
  // --- Configure Timer1 for PWM generation (pins D9, D10) ---
  TCCR1A = 0;            // Timer1: No PWM enabled, OC1A/OC1B normal operation on startup
  TCCR1B = 0x01;         // Timer1: No prescaling, clock selected as system clock (fast PWM rate)
  
  // --- Configure Timer2 for PWM generation (pin D11) ---
  TCCR2A = 0;            // Timer2: No PWM enabled, OC2A normal operation on startup
  TCCR2B = 0x01;         // Timer2: No prescaling, clock selected as system clock
  
  // --- Configure analog comparator ---
  ACSR   = 0x10;         // Analog Comparator: Disable interrupt (ACIE=0), clear interrupt flag (ACI=1)
  
  // --- Configure input buttons with pull-up resistors (active LOW logic) ---
  pinMode(SPEED_UP,   INPUT_PULLUP);   // Motor speed-up button (A0): internal pull-up, LOW when pressed
  pinMode(SPEED_DOWN, INPUT_PULLUP);   // Motor speed-down button (A1): internal pull-up, LOW when pressed
}

// --- Analog Comparator Interrupt Service Routine ---
// Called on comparator event (when zero-crossing detected during BLDC commutation)
// Performs digital debouncing and advances commutation sequence
ISR (ANALOG_COMP_vect) {
  // Debounce: Loop 10 times to ensure stable comparator signal before commutating
  for(i = 0; i < 10; i++) {
    if(bldc_step & 1){                   // For odd commutation steps (phases expecting rising/falling BEMF)
      if(!(ACSR & 0x20)) i -= 1;         // If comparator output is LOW, retry current sample
    }
    else {                               // For even commutation steps
      if((ACSR & 0x20))  i -= 1;         // If comparator output is HIGH, retry current sample
    }
  }
  bldc_move();                           // Run next commutation step (advance phase sequence)
  bldc_step++;                           // Increment commutation step counter (0-5)
  bldc_step %= 6;                        // Wrap-around at step 6 (6-step commutation cycle)
}

// --- BLDC Motor Phase Commutation Function ---
// Advances motor windings in six-step sequence based on current commutation step
void bldc_move(){
  switch(bldc_step){
    case 0:
      AH_BL();            // Phase A High (PWM, D9), Phase B Low (D4 ON), Phase C floating (sense BEMF)
      BEMF_C_RISING();    // Set analog comparator to detect rising edge BEMF for phase C
      break;
    case 1:
      AH_CL();            // Phase A High (PWM, D9), Phase C Low (D3 ON), Phase B floating
      BEMF_B_FALLING();   // Set analog comparator for falling edge BEMF phase B
      break;
    case 2:
      BH_CL();            // Phase B High (PWM, D10), Phase C Low (D3 ON), Phase A floating
      BEMF_A_RISING();    // Set analog comparator for rising edge BEMF phase A
      break;
    case 3:
      BH_AL();            // Phase B High (PWM, D10), Phase A Low (D5 ON), Phase C floating
      BEMF_C_FALLING();   // Set analog comparator for falling edge BEMF phase C
      break;
    case 4:
      CH_AL();            // Phase C High (PWM, D11), Phase A Low (D5 ON), Phase B floating
      BEMF_B_RISING();    // Set analog comparator for rising edge BEMF phase B
      break;
    case 5:
      CH_BL();            // Phase C High (PWM, D11), Phase B Low (D4 ON), Phase A floating
      BEMF_A_FALLING();   // Set analog comparator for falling edge BEMF phase A
      break;
  }
}

void loop() {
  // --- Motor startup sequence ---
  SET_PWM_DUTY(PWM_START_DUTY);      // Set initial motor speed via PWM duty cycle

  i = 5000;                          // Initial delay for slow commutation
  // Gradually accelerate motor by reducing commutation delay
  while(i > 100) {
    delayMicroseconds(i);            // Wait between commutation steps for ramp-up
    bldc_move();                     // Force commutation step, pre-BEMF startup
    bldc_step++;                     // Advance commutation step (0-5)
    bldc_step %= 6;                  // Wrap-around after 6 steps
    i = i - 20;                      // Incrementally decrease delay for acceleration
  }

  motor_speed = PWM_START_DUTY;      // Initialize motor speed variable

  ACSR |= 0x08;                      // Enable Analog Comparator Interrupt (ACIE=1); zero-crossing detection begins

  // --- Runtime: respond to user button input to adjust motor speed ---
  while(1) {
    while(!(digitalRead(SPEED_UP)) && motor_speed < PWM_MAX_DUTY){
      motor_speed++;                 // Increase PWM duty (motor power/speed up)
      SET_PWM_DUTY(motor_speed);     // Update timer compare registers for new speed
      delay(100);                    // Debounce / rate limit for button input
    }
    while(!(digitalRead(SPEED_DOWN)) && motor_speed > PWM_MIN_DUTY){
      motor_speed--;                 // Decrease PWM duty
      SET_PWM_DUTY(motor_speed);     // Update for new duty cycle
      delay(100);                    // Debounce/rate limit
    }
  }
}

// --- Analog Comparator Configuration Functions for Zero-Crossing Detection ---
// These functions dynamically set up the comparator and ADC for the correct BEMF phase

void BEMF_A_RISING(){
  ADCSRB = (0 << ACME);        // Select AIN1 (D7) as comparator negative input; comparator in normal mode
  ACSR |= 0x03;                // Set comparator to fire interrupt on rising edge (ACIS1:0 = 11)
}
void BEMF_A_FALLING(){
  ADCSRB = (0 << ACME);        // Select AIN1 as comparator negative input
  ACSR &= ~0x01;               // Set comparator to fire interrupt on falling edge (ACIS1:0 = 10)
}
void BEMF_B_RISING(){
  ADCSRA = (0 << ADEN);        // Disable ADC (free comparator multiplexer for BEMF phase selection)
  ADCSRB = (1 << ACME);        // Enable analog comparator multiplexer; can select other analog channels
  ADMUX = 2;                   // Set comparator negative input to analog channel 2 (A2)
  ACSR |= 0x03;                // Interrupt on rising edge
}
void BEMF_B_FALLING(){
  ADCSRA = (0 << ADEN);
  ADCSRB = (1 << ACME);
  ADMUX = 2;
  ACSR &= ~0x01;               // Interrupt on falling edge
}
void BEMF_C_RISING(){
  ADCSRA = (0 << ADEN);
  ADCSRB = (1 << ACME);
  ADMUX = 3;                   // Comparator negative input: analog channel 3 (A3)
  ACSR |= 0x03;                // Interrupt on rising edge
}
void BEMF_C_FALLING(){
  ADCSRA = (0 << ADEN);
  ADCSRB = (1 << ACME);
  ADMUX = 3;
  ACSR &= ~0x01;               // Interrupt on falling edge
}

// --- BLDC phase switch functions ---
// Controls which motor windings (phases) are powered and which are grounded

void AH_BL(){
  PORTD &= ~0x28;              // Clear bits for D5 and D3 (Low A, Low B) - ensures these are LOW
  PORTD |=  0x10;              // Set D4 HIGH (Low side of Phase B ON)
  TCCR1A =  0;                 // Turn OFF PWM on D9/D10 (Timer1)
  TCCR2A =  0x81;              // Enable non-inverting PWM on D11 (Timer2, COM2A1=1)
}
void AH_CL(){
  PORTD &= ~0x30;              // Clear bits for D5 and D4
  PORTD |=  0x08;              // Set D3 HIGH
  TCCR1A =  0;
  TCCR2A =  0x81;
}
void BH_CL(){
  PORTD &= ~0x30;
  PORTD |=  0x08;
  TCCR2A =  0;
  TCCR1A =  0x21;              // Enable PWM on D10 (Timer1, COM1B1=1)
}
void BH_AL(){
  PORTD &= ~0x18;              // Clear bits for D4 and D3
  PORTD |=  0x20;              // Set D5 HIGH
  TCCR2A =  0;
  TCCR1A =  0x21;
}
void CH_AL(){
  PORTD &= ~0x18;
  PORTD |=  0x20;
  TCCR2A =  0;
  TCCR1A =  0x81;              // Enable PWM on D9
}
void CH_BL(){
  PORTD &= ~0x28;
  PORTD |=  0x10;
  TCCR2A =  0;
  TCCR1A =  0x81;
}

// --- PWM Duty Cycle Setter ---
// Ensures PWM value is within operational boundaries and updates all phase PWMs accordingly
void SET_PWM_DUTY(byte duty){
  if(duty < PWM_MIN_DUTY)
    duty  = PWM_MIN_DUTY;      // Clamp to minimum allowed duty cycle
  if(duty > PWM_MAX_DUTY)
    duty  = PWM_MAX_DUTY;      // Clamp to maximum allowed duty cycle
  OCR1A  = duty;               // Set duty cycle for PWM output on D9 (Timer1, Phase A high)
  OCR1B  = duty;               // Set duty cycle for PWM output on D10 (Timer1, Phase B high)
  OCR2A  = duty;               // Set duty cycle for PWM output on D11 (Timer2, Phase C high)
}
