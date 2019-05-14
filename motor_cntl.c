#include <stdio.h>
#include <wiringPi.h>
#include <math.h>
#include <sys/types.h> // open
#include <sys/stat.h>  // open
#include <sys/ioctl.h>
#include <fcntl.h>     // open
#include <unistd.h>    // read/write usleep
#include <stdlib.h>    // exit
#include <inttypes.h>  // uint8_t, etc
#include <linux/i2c-dev.h> // I2C bus definitions

int mtr_step_size (int size)
{
   pinMode (0,OUTPUT); //set pin 11 (GPIO GEN17) as output
   pinMode (1,OUTPUT); //set pin 12 (GPIO GEN18) as output
   pinMode (2,OUTPUT); //set pin 13 (GPIO GEN27) as output

   switch(size)
   {
      case 1:  // FULL STEP
         printf("FULL STEP OPTION\n");
         digitalWrite(0,0); //MS1 LOW
         digitalWrite(1,0); //MS2 LOW
         digitalWrite(2,0); //MS3 LOW
      break;
      case 2:  // 1/2 STEP
         printf("1/2 STEP OPTION\n");
         digitalWrite(0,1); //MS1 HIGH
         digitalWrite(1,0); //MS2 LOW
         digitalWrite(2,0); //MS3 LOW
      break;
      case 4:  // 1/4 STEP
         printf("1/4 STEP OPTION\n");
         digitalWrite(0,0); //MS1 LOW
         digitalWrite(1,1); //MS2 HIGH
         digitalWrite(2,0); //MS3 LOW
      break;
      case 8:  // 1/8 STEP
         printf("1/8 STEP OPTION\n");
         digitalWrite(0,1); //MS1 HIGH
         digitalWrite(1,1); //MS2 HIGH
         digitalWrite(2,0); //MS3 LOW
      break;
      case 16:  // 1/16 STEP
         printf("1/16 STEP OPTION\n");
         digitalWrite(0,1); //MS1 HIGH
         digitalWrite(1,1); //MS2 HIGH
         digitalWrite(2,1); //MS3 HIGh
      break;
      default: // FULL STEP
         printf("FULL STEP OPTION\n");
         digitalWrite(0,0); //MS1 LOW
         digitalWrite(1,0); //MS2 LOW
         digitalWrite(2,0); //MS3 LOW
      break;
   }

   return 0;
}

int ui_get_size (void)
{
   char opt[1];
   int flag = 0;
   int size = 0;

   while (!flag) {
      printf("set motor step size: [a] = full, [b] = 1/2, [c] = 1/4, [d] = 1/8; [e] = 1/16\n");
      scanf("%s",opt);
      if (opt[0] == 'a') {
         flag = 1;
	 size = 1;
      } else if (opt[0] == 'b') {
         flag = 1;
	 size = 2;
      } else if (opt[0] == 'c') {
         flag = 1;
	 size = 4;
      } else if (opt[0] == 'd') {
         flag = 1;
	 size = 8;
      } else if (opt[0] == 'e') {
         flag = 1;
	 size = 16;
      } else flag = 0;
   }

   return size;
}

int ui_en_motor (void)
{
   char opt[1];
   int flag = 0;
   int en = 1;

   pinMode (3,OUTPUT); //set pin 15 (GPIO GEN22) as output

   while (!flag) {
      printf("Would you like to enable the stepper motor: [y] = yes, [n] = no\n");
      scanf("%s",opt);
      if (opt[0] == 'y') {
         printf("Motor ENABLED\n");
         flag = 1;
         digitalWrite(3,0); // n_ENABLE LOW
	 en = 1;
      } else if (opt[0] == 'n') {
         printf("Motor DISABLED\n");
         flag = 1;
         digitalWrite(3,1); // n_ENABLE HIGH
	 en = 0;
      } else flag = 0;
   }

   return en;
}

int ui_set_direction (void)
{
   char opt[1];
   int flag = 0;
   int en = 1;

   pinMode (4,OUTPUT); //set pin 16 (GPIO GEN23) as output

   while (!flag) {
      printf("Which direction would you like to turn tepper motor: [r] = clockwise, [l] = counter\n");
      scanf("%s",opt);
      if (opt[0] == 'r') {
         printf("Motor Direction CLOCKWISE\n");
         flag = 1;
         digitalWrite(4,0); // n_ENABLE LOW
	 en = 1;
      } else if (opt[0] == 'l') {
         printf("Motor Direction COUNTER-CLOCKWISE\n");
         flag = 1;
         digitalWrite(4,1); // n_ENABLE HIGH
	 en = 0;
      } else flag = 0;
   }

   return en;
}

int ui_step_num (int size)
{
   int flag = 0;
   int num = 0;
   float deg_step = 0;
   int max_step = 200;
   int i = 0;


   deg_step = 1.8 / size;
   max_step = 200 * size;

   pinMode (5,OUTPUT); //set pin 18 (GPIO GEN24) as output
   digitalWrite(5,0); // Step LOW
   delay(10);

   while (!flag) {
      printf("How steps of %5.4f degrees step would you like to take: [0-%03d]\n", deg_step, max_step);
      scanf("%d",&num);
      if (num >= 0 && num <= max_step) {
         printf("%d Steps\n", num);
         flag = 1;
	 for (i = 0; i < num; i++) {
            digitalWrite(5,1); // Step HIGH
            delay(100);
            digitalWrite(5,0); // Step LOW
            delay(100);
	 }
      } else flag = 0;
   }

   return 0;
}

int adc_step_num (int size, int num)
{
   int max_step = 200;
   int i = 0;

   max_step = 200 * size;

   pinMode (5,OUTPUT); //set pin 18 (GPIO GEN24) as output
   digitalWrite(5,0); // Step LOW
   delay(10);

   if (num >= 0 && num <= max_step) {
      printf("%d Steps\n", num);
      for (i = 0; i < num; i++) {
         digitalWrite(5,1); // Step HIGH
         delay(100);
         digitalWrite(5,0); // Step LOW
         delay(100);
      }
   }

   return 0;
}

int adc_read (){
   int fd;
   // Note PCF8591 defaults to 0x48!
   int ads_address = 0x48;
   int16_t val;
   int steps;
   int valmax = 26347;

   uint8_t writeBuf[3];
   uint8_t readBuf[2];

   float myfloat;

   const float VPS = 4.096 / 32768.0; //volts per step


  // open device on /dev/i2c-1 
  // the default on Raspberry Pi B
  if ((fd = open("/dev/i2c-1", O_RDWR)) < 0) {
    printf("Error: Couldn't open device! %d\n", fd);
    exit (1);
  }

  // connect to ads1115 as i2c slave
  if (ioctl(fd, I2C_SLAVE, ads_address) < 0) {
    printf("Error: Couldn't find device on address!\n");
    exit (1);
  }

  // set config register and start conversion
  // ANC1 and GND, 4.096v, 128s/s
  writeBuf[0] = 1;    // config register is 1
  //bits:         111111
  //              54321098
  writeBuf[1] = 0b11000011; // bit 15-8 0xD3
  // bit 15 flag bit for single shot
  // Bits 14-12 input selection:
  // 100 ANC0 current selection
  // 101 ANC1
  // 110 ANC2
  // 111 ANC3
  // Bits 11-9 Amp gain. Default to
  // 011 +/-1.024V (min)
  // 010 +/-2.048V (Default)
  // 001 +/-4.096V (current)
  // 000 +/-6.144V (max)
  // Bit 8 Operational mode of the ADS1115.
  // 0 : Continuous conversion mode
  // 1 : Power-down single-shot mode (default)

  writeBuf[2] = 0b10000101; // bits 7-0  0x85
  // Bits 7-5 data rate default to 100 for 128SPS
  // Bits 4-0  comparator functions see spec sheet.

  // begin conversion
  if (write(fd, writeBuf, 3) != 3) {
    perror("Write to register 1");
    exit(-1);
  }

  // wait for conversion complete
  // checking bit 15
  do {
    if (read(fd, writeBuf, 2) != 2) {
      perror("Read conversion");
      exit(-1);
    }
  }
  while ((writeBuf[0] & 0x80) == 0);



  // read conversion register
  // write register pointer first
  readBuf[0] = 0;   // conversion register is 0
  if (write(fd, readBuf, 1) != 1) {
    perror("Write register select");
    exit(-1);
  }
  
  // read 2 bytes
  if (read(fd, readBuf, 2) != 2) {
    perror("Read conversion");
    exit(-1);
  }

  // convert display results
  val = readBuf[0] << 8 | readBuf[1];

  if (val < 0)   val = 0;

  myfloat = val * VPS; // convert to voltage

  printf("Values: HEX 0x%02x DEC %d reading %4.3f volts.\n",
         val, val, myfloat);

  steps = ((val * 200) /  valmax);
//  printf("steps = %d*****\n",steps);       
//  steps = 50;
  close(fd);

  return (steps);   
}

int comp_read (int* x_read, int* y_read, int* z_read){
  // Create I2C bus
  int file;
  char *bus = "/dev/i2c-1";
  char config [2] = {0};
  char reg [1] = {0x28};
  char data [1] = {0};

  if((file = open(bus, O_RDWR)) < 0) {
     printf("Failed to open the bus. \n");
     exit(1);
  }

  // Select MR register(0x02)
  // Continuous conversion(0x00)
  config[0] = 0x02; // ADDDR
  config[1] = 0x00; // DATA
  write(file, config, 2);

  // Select CRA register(0x00)
  // Data output rate = 220Hz(0x1C)
  config[0] = 0x00; // ADDDR
  config[1] = 0x1C; // DATA
  write(file, config, 2);

  // Select CRB register(0x01)
  // Set gain = +/- 8.1g(0xE0)
  config[0] = 0x01; // ADDDR
  config[1] = 0xE0; // DATA
  write(file, config, 2);
  sleep(1);

  // Read 6 bytes of data
  // msb first
  // Read xMag msb data from register(0x03)
  reg[0] = 0x03;
  write(file, reg, 1);
  read(file, data, 1);
  char data1_0 = data[0];

  // Read xMag lsb data from register(0x04)
  reg[0] = 0x04;
  write(file, reg, 1);
  read(file, data, 1);
  char data1_1 = data[0];

  // Read yMag msb data from register(0x05)
  reg[0] = 0x05;
  write(file, reg, 1);
  read(file, data, 1);
  char data1_2 = data[0];

  // Read yMag lsb data from register(0x06)
  reg[0] = 0x06;
  write(file, reg, 1);
  read(file, data, 1);
  char data1_3 = data[0];

  // Read zMag msb data from register(0x07)
  reg[0] = 0x07;
  write(file, reg, 1);
  read(file, data, 1);
  char data1_4 = data[0];

  // Read zMag lsb data from register(0x08)
  reg[0] = 0x08;
  write(file, reg, 1);
  read(file, data, 1);
  char data1_5 = data[0];

  // Convert the data
  int xMag = (data1_0 * 256 + data1_1);
  if(xMag > 32767) {
     xMag -= 65536;
  }	

  int yMag = (data1_4 * 256 + data1_5) ;
  if(yMag > 32767) {
     yMag -= 65536;
  }

  int zMag = (data1_2 * 256 + data1_3) ;
  if(zMag > 32767) {
     zMag -= 65536;
  }

  // Output data to screen
/*  printf("Magnetic field in X-Axis : %d \n", xMag);
  *x_read = xMag;
  printf("Magnetic field in Y-Axis : %d \n", yMag);
  *y_read = yMag;
  printf("Magnetic field in Z-Axis : %d \n", zMag);
  *z_read = zMag;
*/
  return (0);   
}

int main (void)
{
   int opt, step;
   int en;
   char ui_opt[1];
   int flag = 0;
   int x_start, x_end;
   int y_start, y_end;
   int z_start, z_end;
   float calc_angle, motor_angle;

   printf ("Motor CNTL BEGIN\n");

   if (wiringPiSetup() == -1){
      printf("wiringPi setup failed\n");
      return 1;
   }

   en = ui_en_motor();
   if (en) {
      opt = ui_get_size();
      mtr_step_size (opt);
      comp_read (&x_start, &y_start, &z_start);
      while (!flag) {
         ui_set_direction();
         step = adc_read();
         adc_step_num (opt, step);
         comp_read (&x_end, &y_end, &z_end);
	 motor_angle = (step*(1.8*opt));
	 calc_angle = (atan2((y_end - y_start), (x_end - x_start)) * 180) / 3.14;
	 if (calc_angle < 0) calc_angle = calc_angle + 360;
         printf("motor_angle = %6.3f\tcalc_angle = %6.3f\n",motor_angle,motor_angle); 
         printf("Would you like to quit: [y] = yes, [n] = no\n");
         scanf("%s",ui_opt);
         if (ui_opt[0] == 'y') {
            printf("QUITTING\n");
            flag = 1;
            digitalWrite(0,0); // n_ENABLE LOW
            digitalWrite(1,0); // n_ENABLE LOW
            digitalWrite(2,0); // n_ENABLE LOW
            digitalWrite(3,0); // n_ENABLE LOW
            digitalWrite(4,0); // n_ENABLE LOW
            digitalWrite(5,0); // n_ENABLE LOW
         } else if (ui_opt[0] == 'n') {
            printf("Run Motor\n");
            flag = 0;
         } else flag = 0;
      }
   }

   printf ("Motor CNTL END\n");
   return 0;
}
