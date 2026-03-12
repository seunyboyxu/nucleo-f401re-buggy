#include "mbed.h"
#include "C12832.h"
#include "math.h"
#include "QEI.h"
#define _USE_MATH_DEFINES
#include <cmath>


class Potentiometer                                                     //Begin updated potentiometer class definition
    {

    private:                                                                //Private data member declaration
        AnalogIn inputSignal;                                               //Declaration of AnalogIn object
        float VDD, currentSampleNorm, currentSampleVolts;    
                     //Float variable to speficy the value of VDD (3.3 V for the Nucleo-64)

    public:                                                                 // Public declarations
        Potentiometer(PinName pin, float v) : inputSignal(pin), VDD(v) {}   //Constructor - user provided pin name assigned to AnalogIn...
                                                                            //VDD is also provided to determine maximum measurable voltage
        float amplitudeVolts(void)                                          //Public member function to measure the amplitude in volts
        {
            return (inputSignal.read()*VDD);                                //Scales the 0.0-1.0 value by VDD to read the input in volts
        }

        float amplitudeNorm(void)                                           //Public member function to measure the normalised amplitude
        {
            return inputSignal.read();                                      //Returns the ADC value normalised to range 0.0 - 1.0
        }

        void sample(void)                                                   //Public member function to read a sample and store the value as data members
        {
            currentSampleNorm = inputSignal.read();                         //Read a sample from the ADC and store normalised representation [0..1]
            currentSampleVolts = currentSampleNorm*VDD;                     //Convert this to a voltage and store that as a data member too.
        }

        float getCurrentSampleNorm(void)                                   //Public member function to return the most recent normalised sample [0..1]
        {
            return currentSampleNorm;                                       //Return the most recent normalised sample
        }

        float getCurrentSampleVolts(void)                                   //Public member function to return the most recent sampled voltage [0.. 3.3 V]
        {
            return currentSampleVolts;                                      //Return the most recent sampled voltage
        }

};

class SamplingPotentiometer : public Potentiometer{

    private:
        float samplingFrequency, samplingPeriod;
        Ticker sampler;

    public:
        SamplingPotentiometer(PinName p, float v, float fs)
            : Potentiometer(p, v), samplingFrequency(fs), samplingPeriod(1.0f / samplingFrequency)
    {
        sampler.attach(callback(this, &Potentiometer::sample), samplingPeriod);
    }
};

C12832 LCD(D11, D13, D12, D7, D10); // LCD initialisation

//state machine
typedef enum {DUTY_CYCLE, PULSES, BLUETOOTH, SENSOR, IDLE} ProgramState;
volatile ProgramState state = DUTY_CYCLE;

void fireISR()
{
    switch (state)
    {
        case(DUTY_CYCLE):
            state = PULSES;
            break;
        case(PULSES):
            state = BLUETOOTH;
            break;
        case(BLUETOOTH):
            state = SENSOR;
            break;
        case(SENSOR):
            state = IDLE;
            break;
        case(IDLE):
            state = DUTY_CYCLE;
            break;
    }
}



SamplingPotentiometer Left_P(A0, 3.3, 100);
SamplingPotentiometer Right_P(A1, 3.3, 100); // objects for motor control

class BuggyWheel
{
    private: 
        float duty, direction, period;
        PwmOut PWM_Output;
        DigitalOut Bipolar, DirectionPin;
        
        // 1. Ticker is now a private member of the wheel
        Ticker encoder_ticker;
        
        // 2. Variables modified in an ISR must be volatile
        volatile float Current_Pulses;
        volatile float Previous_Pulses;
        
        // 3. Keep track of the absolute count to avoid resetting the encoder
        volatile int absolute_pulses_prev; 

        //rmp and velocity
        float rpm, velocity;
        float r;
        double pi;

    public:
        QEI encoder;
        float sample_rate;
        
        // Constructor updated to take a sample rate (frequency in Hz)
        BuggyWheel(PinName pwmpin, PinName biplr, PinName Dir, PinName Chan_A, PinName Chan_B, float sample_hz = 100.0f) 
            : PWM_Output(pwmpin), 
              Bipolar(biplr), 
              DirectionPin(Dir),
              Current_Pulses(0),
              Previous_Pulses(0),
              absolute_pulses_prev(0),
              encoder(Chan_A, Chan_B, NC, 256, QEI::X2_ENCODING),
              sample_rate(sample_hz)
        {
            r = 0.0778;
            pi = 3.14159265358979323846;

        }

        void SetUp()
        {
            Bipolar.write(1); // enable bipolar mode
            DirectionPin.write(0);
            PWM_Output.period(0.00005f);
            duty = 0.5; // enable set direction
            PWM_Output.write(duty);
            
            // Attach the ticker callback during setup. 
            // Period is 1.0 / frequency (in seconds as float).
            encoder_ticker.attach(callback(this, &BuggyWheel::Measure_Pulses), 1.0f / sample_rate);
        }
        
        int GetPulses()
        {  
            return encoder.getPulses();
        }

        void SetDuty(float d)
        {
            duty = d;
        }

        void Brake()
        {
            duty = 0.5;
            PWM_Output.write(duty);
        }

        void Move(double a)
        {
            PWM_Output.write(duty + a);
        }

        void ChangeDuty()
        {
            PWM_Output.write(duty);
        }

        // This is now called automatically by the class's internal Ticker
        void Measure_Pulses()
        {
            int current_absolute = this->encoder.getPulses();
            int delta_pulses = current_absolute - absolute_pulses_prev;
            this->absolute_pulses_prev = current_absolute;
            
            this->Previous_Pulses = this->Current_Pulses;
            
            // Multiply by sample_rate to always output Pulses Per Second (Hz)
            // Even if the ticker runs at 10Hz or 50Hz, this maths stays accurate!
            this->Current_Pulses = (float)delta_pulses * sample_rate; 
        }

        float Tell_CPulses()
        {
            return Current_Pulses;
        }

        float Tell_PPulses()
        {
            return Previous_Pulses;
        } 

        // void rpm_calc()
        // {
        //     float pulses;
        //     pulses = abs(this->Current_Pulses);
        //     this->rpm = pulses / 512.0f;
        //     this->rpm *= 60;
        //     this->rpm = Current_Pulses;
        // }

        // float get_rpm() {return rpm;}

        // void velocity_calc()
        // {
        //     this->velocity = (2*pi*r*this->rpm)/60;
        // }

        // float get_vel(){return velocity;}

};

//enable motor
DigitalOut EnableMotor(PC_4);
// Initialize with a 1 Hz sample rate (f = 1 in your original main)
BuggyWheel RightWheel(PC_6, PB_13, PB_14, PB_8, PB_12, 1.0f);
BuggyWheel LeftWheel(PC_8, PB_15, PB_1, PC_10, PC_12, 1.0f);

//Dual Motor control for turning
void Forward(int time)
{
    double r_adjustment = 0;
    double l_adjustment = 0;
    RightWheel.SetDuty(0.359999);
    LeftWheel.SetDuty(0.344);
    RightWheel.Move(r_adjustment);
    LeftWheel.Move(l_adjustment);
    wait_ms(time);
    RightWheel.Brake();
    LeftWheel.Brake();
}

void Turn_Right(int time)
{
    double r_adjustment = 0;
    double l_adjustment = 0;
    RightWheel.SetDuty(0.65);
    LeftWheel.SetDuty(0.35);
    RightWheel.Move(r_adjustment);
    LeftWheel.Move(l_adjustment);
    wait_ms(time);
    RightWheel.Brake();
    LeftWheel.Brake();
}

void Turn_Left(int time)
{
    double r_adjustment = 0;
    double l_adjustment = 0;
    RightWheel.SetDuty(0.35);
    LeftWheel.SetDuty(0.65);
    RightWheel.Move(r_adjustment);
    LeftWheel.Move(l_adjustment);
    wait_ms(time - 50);
    RightWheel.Brake();
    LeftWheel.Brake();
}

void Turn_Around(int time)
{
    double r_adjustment = 0;
    double l_adjustment = 0;
    RightWheel.SetDuty(0.65);
    LeftWheel.SetDuty(0.35);
    RightWheel.Move(r_adjustment);
    LeftWheel.Move(l_adjustment);
    wait_ms(time * 2);
    RightWheel.Brake();
    LeftWheel.Brake();
}


void LCD_Text(float duty_Right, float duty_Left)
{
    float duty_R, duty_L, R_text, L_text;
    duty_R = duty_Right;
    duty_L = duty_Left;
    
    if (duty_R >= 0 && duty_R < 0.5){
        R_text = 1 - (duty_R*2);
    }
    else if (duty_R > 0.5 && duty_R <= 1){
        R_text = 2 - duty_R * 2;
        R_text = 1 - R_text;
        R_text *= -1;
    }
    else {
        R_text = 0.00;
    }
    if (duty_L >= 0 && duty_L < 0.5){
        L_text = 1 - (duty_L*2);
    }
    else if (duty_L > 0.5 && duty_L <= 1){
        L_text = 2 - duty_L * 2;
        L_text = 1 - L_text;
        L_text *= -1;
    }
    else {
        L_text = 0.00;
    }
    LCD.cls();
    LCD.locate(15,2);                                       // prints period for motors
    LCD.printf("Period");
    LCD.locate(60,2);
    LCD.printf("0.05 ms"); 
    LCD.locate(95,2);
    LCD.printf("0.05 ms"); 
    LCD.locate(15, 13);                                     // prints duty cycle for motors
    LCD.printf("Duty Cycle:");
    LCD.locate(70, 13);
    LCD.printf("%.2f", L_text);
    LCD.locate(95, 13);
    LCD.printf("%.2f", R_text);                
}

void encoder_text(float L_Pulses, float R_Pulses)
{
    //LeftWheel.rpm_calc();
    //RightWheel.rpm_calc();
    float L_RPM, R_RPM;
    L_Pulses = abs(L_Pulses);
    R_Pulses = abs(R_Pulses);

    L_RPM = L_Pulses / 512.0f;
    L_RPM *= 60;

    R_RPM = R_Pulses / 512.0f;
    R_RPM *= 60;
    //L_RPM = LeftWheel.get_rpm();
    //R_RPM = RightWheel.get_rpm();
    LCD.cls();
    LCD.locate(1,2);
    LCD.printf("RPM/sec: %0.2f   %0.2f ", L_RPM, R_RPM);

    //RightWheel.velocity_calc();
    //LeftWheel.velocity_calc();
    float L_Vel, R_Vel;
    float r = 0.0778;
    double pi = 3.14159265358979323846;
    L_Vel = (2*pi*r*L_RPM)/60;
    R_Vel = (2*pi*r*R_RPM)/60;
    //L_Vel = LeftWheel.get_vel();
    //R_Vel = RightWheel.get_vel();
    LCD.locate(1, 20);
    LCD.printf("m/s: %0.2f   %0.2f ", L_Vel, R_Vel);
}

Serial hm10(PA_11, PA_12);

char c;
void BluetoothConnectionDisplay()
{
    LCD.cls();

    while(1) {
        LCD.locate(1, 2);
        if(hm10.readable()){
            c = hm10.getc(); //read a single character
            if(c == 'A')
            {
                LCD.printf("Pulse received: %c", c);
                
            }
            else if(c == 'B')
            {
                LCD.printf("Pulse received: %c", c);
            }
        }
    }

}



int main() 
{ 
     //enable motor drive board
    RightWheel.SetUp();
    LeftWheel.SetUp();
    EnableMotor.write(1);

    float duty_R = 0.00, duty_L = 0.00, f = 1; 
    float RPM_R, RPM_L;
    float R_Current_Pulses = 0, L_Current_Pulses = 0;

    //set bluetooth baud rate
    hm10.baud(9600);

    //fire interrupt
    InterruptIn fire(D4);
    fire.rise(&fireISR);

    while(1)
    {

  
    switch(state)
    {
        case DUTY_CYCLE:
            duty_R = Right_P.getCurrentSampleNorm();
            duty_L = Left_P.getCurrentSampleNorm();
            RightWheel.SetDuty(duty_R);
            LeftWheel.SetDuty(duty_L);
            RightWheel.ChangeDuty();                 // speed control for right wheel
            LeftWheel.ChangeDuty();
            LCD_Text(duty_R, duty_L);
            wait_ms(50);
            break;
        case PULSES:
            duty_R = Right_P.getCurrentSampleNorm();
            duty_L = Left_P.getCurrentSampleNorm();
            RightWheel.SetDuty(duty_R);
            LeftWheel.SetDuty(duty_L);
            RightWheel.ChangeDuty();                 // speed control for right wheel
            LeftWheel.ChangeDuty();
            L_Current_Pulses = LeftWheel.Tell_CPulses();
            R_Current_Pulses = RightWheel.Tell_CPulses();
            encoder_text(L_Current_Pulses, R_Current_Pulses);
            wait_ms(30);
            break;
        case BLUETOOTH:
            //square movement test
            LCD.cls();
            // SquareTest(1020, 470);
            BluetoothConnectionDisplay();
            break;

        case SENSOR:

            break;
        default:
            break;



    }       
    }
}
