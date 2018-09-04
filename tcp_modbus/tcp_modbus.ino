/*
#You can send commands from python with this code:
#It needs the pymodbus library installed


from pymodbus.client.sync import ModbusTcpClient
client=ModbusTcpClient('192.168.0.2')
client.write_coil(1,True)
client.close()
*/

#include "Ethernet.h"



#define TIMEOUT 1000  //Connection timeout in milisegundos
#define DEBUG false
#define BUFFERSIZE 256

void  ProcesarComando(int);





EthernetServer server(502);
char bufferEntrada[BUFFERSIZE];
int bytesRecibidos;
char bufferSalida[BUFFERSIZE];
int bytesEnviados;



void setup() {
  Serial.begin(115200);  
  delay(1000);  
  Serial.println("debug channel initialized");
  
  if (DEBUG) Serial.println("al inicio de setup");
  IPAddress ip = IPAddress(192,168,0,2);
  IPAddress dns = IPAddress(193,145,233,5);
  IPAddress gw = IPAddress(192,168,0,1);
  IPAddress mask = IPAddress(255,255,255,0);

  //Ethernet.begin(0); si se quiere usar DHCP
  Ethernet.begin(0, ip, dns, gw);

  server.begin();


  //salidas digitales
  //coils modbus. Solo se permiten 32 coils distintas para poder almacenar sus valores en un unsigned long
  //pinMode(PM_6, OUTPUT);
  //digitalWrite(PM_6, LOW);
  
  //entradas digitales
  //pinMode(PM_6, INPUT);

  

}

EthernetClient client;

void loop() {

  int i=0;  //generica para loops
  int longitudComando=0;

  client = server.available();
  
  if (client) {                             // if you get a client,
    if (DEBUG) Serial.print("new client on port ");           // print a message out the serial port
    if (DEBUG) Serial.println(client.port());
    String currentLine = "";                // make a String to hold incoming data from the client
    boolean newConnectionFlag = true;     // flag for new connections
    unsigned long connectionActiveTimer;  // will hold the connection start time
    boolean receibedHeaderFlag= false;   //aun no se ha recibido la cabecera tcp donde esta la longitud del comando

    
    while (client.connected()) {       // loop while the client's connected
      if (newConnectionFlag){                 // it's a new connection, so
        bytesRecibidos=0;
        receibedHeaderFlag=false;
        connectionActiveTimer = millis(); // log when the connection started
        newConnectionFlag = false;          // not a new connection anymore
      }
      if (!newConnectionFlag && connectionActiveTimer + TIMEOUT < millis()){ 
        //La conexion no es nueva y ha expirado el timeout -> error de timeout, es hora de dar el mensaje por erroneo
        if (DEBUG) Serial.println("Conexion terminada por Timeout");
        break;  
      }


      if (client.available()) {             //hay bytes por leer en el bus de comunicaciones ethernet    
        char c = client.read();             
        bufferEntrada[bytesRecibidos]=c;
        bytesRecibidos++;
        if ( (bytesRecibidos>=7)&&(!receibedHeaderFlag) ) {
          //los 7 primeros bytes recibidos son la cabecera MBAP. Es recopiada en la respuesta excepto los
          //bytes con direccion 4 y 5 que son el numero de bytes que faltan por llegar, el total de bytes de la trama es ese numero +6 que ya han llegado 
          receibedHeaderFlag=true;
          longitudComando=(int) ((        (((int) bufferEntrada[4])<<8)             )|(      ((int) bufferEntrada[5])          ))+6;
          if (DEBUG) Serial.print("Longitud de comando es:  ");
          if (DEBUG) Serial.println(longitudComando);
        }
        if ( (receibedHeaderFlag)&&(bytesRecibidos>=(longitudComando)) ) {
          if (DEBUG) {
            Serial.println("Comando recibido");
            for (i=0;i<longitudComando;i++) {
              Serial.print(" 0x");
              Serial.print(bufferEntrada[i],HEX);
            }
            Serial.print("\n");
          }
          ProcesarComando(longitudComando);
          break;
        }
      }
    }
    // close the connection:
    client.stop();
    if (DEBUG) Serial.println("client disonnected");

  } else {
    //AQUI TAREAS DE A/D D/A ETC
    //midiendo con micros() el tiempo desde que se detecta un cliente modbus hasta que se envia el comando de respuesta sale entre 100 y 400us 
    //asi que entre que se sale de aquí y se vuelve a entrar no debería pasar más de eso. 1ms máximo
    //por otra parte esta rutina no deberia retrasarse más de 1s, ya que el timeout de TCP Modbus suele ser mayor de 1s. Debería ser suficiente para la mayoria. 
    //así se podria evitar usar multitarea
    //por otra parte cuanto mas se tarde aqui mas retardos en el canal tcp que es comun para todos los aparatos
    //se deberia bajar al maximo posible
    //se pueden usar interrupciones para detectar flancos en bits de entrada que pueden usarse para hacer la lectura de datos desde A/D o DAC por interrupcion
    //o bien hacerlo aqui pero intentando reducir en varias funciones la tarea y luego ejecutando cada funcion en un tiempo concreto. Por ejemplo:
    
    /* hace falta N tareas muy rapidas pero hay que esperar mucho entre tarea y tarea. Se puede hacer asi
     * nTarea será 1 cuando se ejecute esto por primera vez
     * switch (nTarea):
     *   case 1:
     *     if (millis()-t0)>retraso1) {
     *       tarea1();
     *       t0=millis();
     *       tarea=2;
     *     }  
     *     break;
     *   case 2:
     *     if (millis()-t0)>retraso2) {
     *       tarea2();
     *       t0=millis();
     *       tarea=3;
     *     }  
     *     break;
     *     
     *     ...
     *     
     *   case N:
     *     if (millis()-t0)>retrasoN) {
     *       tareaN();  //esta es la ultima tarea y deberia recoger los datos obtenidos y ponerlos a disposicion de modbus. Como no se usan interrupciones no hay que
     *                  //lidiar con exclusion mutua de tareas...
     *       t0=millis();
     *       tarea=0;
     *     }  
     *     break;
     *     
     */

     //tambien se puede usar multitarea pero TIVA C Connected no lo permite, hay que irse a tarjetas mas potentes.
    
    
  }
  


}


void ProcesarComando(int longitudComando) {
  //procesar el comando
  int i;
  unsigned int direccionBase=0;
  unsigned int nCoils=0;
  boolean valorBobina=false;
  unsigned long valorTodasBobinas=0;
  unsigned int nDin=0;
  boolean valorDin=false;
  unsigned long valorTodasDin=0;

  unsigned int nRegIn=0;
  unsigned int valorRegIn=0; 
  unsigned int OutputValue=0;

  
  switch ((int) bufferEntrada[7]) {  //los primeros 7 bytes son la cabecera TCP que se copiara en la respuesta
    case 1: //Leer bobinas
    //programa ejemplo para enviar un comando de este tipo desde python
    //from pymodbus.client.sync import ModbusTcpClient
    //client=ModbusTcpClient('192.168.0.2')
    //response=client.read_coils(1)
    //client.close()
    //print response.bits[0]
    //
      direccionBase=(unsigned int) bufferEntrada[8];
      direccionBase=direccionBase<<8;
      direccionBase=direccionBase|((unsigned int) bufferEntrada[9]);

      nCoils=(unsigned int) bufferEntrada[10];
      nCoils=nCoils<<8;
      nCoils=nCoils|((unsigned int) bufferEntrada[11]);
      if (nCoils>32) nCoils=32; //de momento solo se pueden 32 bobinas para que sus valores quepan en un unsigned long
      valorTodasBobinas=0;
      
      for (i=0;i<nCoils;i++) {
        //leer el valor de las bobinas,
        //BOBINA 0 -> NADA, ANTES PM_6
        //NO HAY MAS
        switch (i+direccionBase) {
//          case 0:
//            valorBobina=digitalRead(PM_6);
//            break;
          default:
            valorBobina=0;
        }
        if (valorBobina>1) valorBobina=0;
        valorTodasBobinas=(valorTodasBobinas<<1);
        if (valorBobina) valorTodasBobinas=valorTodasBobinas|1;
      }
      //la cabecera se copia
      for (i=0;i<7;i++) bufferSalida[i]=bufferEntrada[i];
      bufferSalida[5]=4; //bytes que quedan en la respuesta si solo hay 1 byte de datos
      //codigo de funcion
      bufferSalida[7]=0x01;
      //el siguiente byte da el numero de bytes de datos
      bufferSalida[8]=1;
      bufferSalida[9]=(valorTodasBobinas     )&0x000000FF;
      if (nCoils>8)  {
        bufferSalida[5]++;
        bufferSalida[8]++;
        bufferSalida[10]=(valorTodasBobinas>>8  )&0x000000FF;
      }
      if (nCoils>16) {
        bufferSalida[5]++;
        bufferSalida[8]++;
        bufferSalida[11]=(valorTodasBobinas>>16)&0x000000FF;
      }
      if (nCoils>24) {
        bufferSalida[5]++;
        bufferSalida[8]++;
        bufferSalida[12]=(valorTodasBobinas>>24)&0x000000FF;
      }
      bytesEnviados=bufferSalida[5]+6;
      server.write(bufferSalida,bytesEnviados);
      
      if (DEBUG) {
        Serial.print("direccion base de bobinas a leer: 0x");
        Serial.println(direccionBase,HEX);
        Serial.print("Numero de bobinas a leer: 0x");
        Serial.println(nCoils,HEX);
        Serial.println("respuesta enviada: ");
        for (i=0;i<bytesEnviados;i++) {
          Serial.print(" 0x");
          Serial.print(bufferSalida[i],HEX);
          Serial.print("\n");
        }
        
      }
      
      break;
    case 2: //leer entradas digitales copiado de codigo 1
    //programa ejemplo para enviar un comando de este tipo desde python
    //from pymodbus.client.sync import ModbusTcpClient
    //client=ModbusTcpClient('192.168.0.2')
    //leer 12 bits de entrada a partir de la direccion 1
    //response=client.read_discrete_inputs(1,12) 
    //client.close()
    //print response.bits[0]
    //    
      direccionBase=(unsigned int) bufferEntrada[8];
      direccionBase=direccionBase<<8;
      direccionBase=direccionBase|((unsigned int) bufferEntrada[9]);

      nDin=(unsigned int) bufferEntrada[10];
      nDin=nDin<<8;
      nDin=nDin|((unsigned int) bufferEntrada[11]);
      if (nDin>32) nDin=32; //de momento solo se pueden 32 entradas discretas para que sus valores quepan en un unsigned long
      valorTodasDin=0;
      
      for (i=0;i<nDin;i++) {
        //leer el valor de las entradas,
        //Entrada 0 -> NINGUNA, ANTES PM_6
        //NO HAY MAS
        switch (i+direccionBase) {
//          case 0:
//            valorDin=digitalRead(PM_6);
//            break;
          default:
            valorDin=0;
        }
        if (valorDin>1) valorDin=0;
        valorTodasDin=(valorTodasDin<<1);
        if (valorDin) valorTodasDin=valorTodasDin|1;
      }
      //la cabecera se copia
      for (i=0;i<7;i++) bufferSalida[i]=bufferEntrada[i];
      bufferSalida[5]=4; //bytes que quedan en la respuesta si solo hay 1 byte de datos
      //codigo de funcion
      bufferSalida[7]=0x02;
      //el siguiente byte da el numero de bytes de datos
      bufferSalida[8]=1;
      bufferSalida[9]=(valorTodasDin     )&0x000000FF;
      if (nDin>8)  {
        bufferSalida[5]++;
        bufferSalida[8]++;
        bufferSalida[10]=(valorTodasDin>>8  )&0x000000FF;
      }
      if (nDin>16) {
        bufferSalida[5]++;
        bufferSalida[8]++;
        bufferSalida[11]=(valorTodasDin>>16)&0x000000FF;
      }
      if (nDin>24) {
        bufferSalida[5]++;
        bufferSalida[8]++;
        bufferSalida[12]=(valorTodasDin>>24)&0x000000FF;
      }
      bytesEnviados=bufferSalida[5]+6;
      server.write(bufferSalida,bytesEnviados);
      
      if (DEBUG) {
        Serial.print("direccion base de entradas digitales a leer: 0x");
        Serial.println(direccionBase,HEX);
        Serial.print("Numero de entradas digitales a leer: 0x");
        Serial.println(nDin,HEX);
        Serial.println("respuesta enviada: ");
        for (i=0;i<bytesEnviados;i++) {
          Serial.print(" 0x");
          Serial.print(bufferSalida[i],HEX);
          Serial.print("\n");
        } 
      }      
      break;
    case 3: //leer registros internos (2 bytes cada uno)
    //programa ejemplo para enviar un comando de este tipo desde python
    //from pymodbus.client.sync import ModbusTcpClient
    //client=ModbusTcpClient('192.168.0.2')
    //leer 3 registros de solo lectura de 16 bits a partir de la direccion 7
    //response=client.read_holding_registers(7,3) 
    //client.close()
    //print response.registers[0]
    //    
      direccionBase=(unsigned int) bufferEntrada[8];
      direccionBase=direccionBase<<8;
      direccionBase=direccionBase|((unsigned int) bufferEntrada[9]);

      nRegIn=(unsigned int) bufferEntrada[10];
      nRegIn=nRegIn<<8;
      nRegIn=nRegIn|((unsigned int) bufferEntrada[11]);
      if (nRegIn>125) nRegIn=125; //el protocolo solo permite 125 registros al mismo tiempo
      for (i=0;i<nRegIn;i++) {
        //leer el valor de los registros
        switch (i+direccionBase) {
//          case 0:
//            valorRegIn=0x341A;
//            break;
          default:
            valorRegIn=i+direccionBase;
        }
        bufferSalida[9+i*2]=(valorRegIn>>8);
        bufferSalida[10+i*2]=(valorRegIn&0x00FF);
      }
      //la cabecera se copia
      for (i=0;i<7;i++) bufferSalida[i]=bufferEntrada[i];
      bufferSalida[5]=3+2*nRegIn; //bytes que quedan en la respuesta
      //codigo de funcion
      bufferSalida[7]=0x03;
      //el siguiente byte da el numero de bytes de datos
      bufferSalida[8]=2*nRegIn;
      bytesEnviados=bufferSalida[5]+6;
      server.write(bufferSalida,bytesEnviados);
      
      if (DEBUG) {
        Serial.print("direccion base de registros a leer: 0x");
        Serial.println(direccionBase,HEX);
        Serial.print("Numero de registros a leer: 0x");
        Serial.println(nRegIn,HEX);
        Serial.println("respuesta enviada: ");
        for (i=0;i<bytesEnviados;i++) {
          Serial.print(" 0x");
          Serial.print(bufferSalida[i],HEX);
          Serial.print("\n");
        } 
      }      
      break;
    case 4: //read input registers
    //es identico a la funcion 3 pero estos son registros asociados a entradas (por ejemplo entradas A/D) en lugar de ser registros de configuracion
    //programa ejemplo para enviar un comando de este tipo desde python
    //from pymodbus.client.sync import ModbusTcpClient
    //client=ModbusTcpClient('192.168.0.2')
    //leer 3 registros de solo lectura de 16 bits a partir de la direccion 7
    //response=client.read_input_registers(7,3) 
    //client.close()
    //print response.registers[0]
    //    
      direccionBase=(unsigned int) bufferEntrada[8];
      direccionBase=direccionBase<<8;
      direccionBase=direccionBase|((unsigned int) bufferEntrada[9]);

      nRegIn=(unsigned int) bufferEntrada[10];
      nRegIn=nRegIn<<8;
      nRegIn=nRegIn|((unsigned int) bufferEntrada[11]);
      if (nRegIn>125) nRegIn=125; //el protocolo solo permite 125 registros al mismo tiempo
      for (i=0;i<nRegIn;i++) {
        //leer el valor de los registros
        switch (i+direccionBase) {
//          case 0:
//            valorRegIn=0x341A;
//            break;
          default:
            valorRegIn=i+direccionBase;
        }
        bufferSalida[9+i*2]=(valorRegIn>>8);
        bufferSalida[10+i*2]=(valorRegIn&0x00FF);
      }
      //la cabecera se copia
      for (i=0;i<7;i++) bufferSalida[i]=bufferEntrada[i];
      bufferSalida[5]=3+2*nRegIn; //bytes que quedan en la respuesta
      //codigo de funcion
      bufferSalida[7]=0x04;
      //el siguiente byte da el numero de bytes de datos
      bufferSalida[8]=2*nRegIn;
      bytesEnviados=bufferSalida[5]+6;
      server.write(bufferSalida,bytesEnviados);
      
      if (DEBUG) {
        Serial.print("direccion base de registros a leer: 0x");
        Serial.println(direccionBase,HEX);
        Serial.print("Numero de registros a leer: 0x");
        Serial.println(nRegIn,HEX);
        Serial.println("respuesta enviada: ");
        for (i=0;i<bytesEnviados;i++) {
          Serial.print(" 0x");
          Serial.print(bufferSalida[i],HEX);
          Serial.print("\n");
        } 
      }      
      break;
    case 5: //escribir una sola bobina
    //programa ejemplo para enviar un comando de este tipo desde python
    //from pymodbus.client.sync import ModbusTcpClient
    //client=ModbusTcpClient('192.168.0.2')
    //escribir true en la bobina 2 y false en la 4
    //response1=client.write_coil(2,True)
    //response2=client.write_coil(4,False) 
    //client.close()
    //    
      direccionBase=(unsigned int) bufferEntrada[8];
      direccionBase=direccionBase<<8;
      direccionBase=direccionBase|((unsigned int) bufferEntrada[9]);

      OutputValue=(unsigned int) bufferEntrada[10];
      OutputValue=OutputValue<<8;
      OutputValue=OutputValue|((unsigned int) bufferEntrada[11]);
      switch (direccionBase) {
      //aqui se debe añadir un caso por cada bobina que efectivamente se tenga y escribir en el bit hardware correspondiente
        default:
            if (DEBUG) {
              Serial.print("la bobina ");
              Serial.print(direccionBase);
              Serial.print("se ha puesto a ");
              Serial.print(OutputValue==0xFF00);
              Serial.print("\n");
            }
      }
      //la cabecera se copia
      for (i=0;i<7;i++) bufferSalida[i]=bufferEntrada[i];
      bufferSalida[5]=6; //bytes que quedan en la respuesta
      //codigo de funcion
      bufferSalida[7]=0x05;
      //los siguientes dos bytes es la direccion de la bobina, que estaban en el buffer de entrada en el mismo sitio
      bufferSalida[8]=bufferEntrada[8];
      bufferSalida[9]=bufferEntrada[9];
      bufferSalida[10]=bufferEntrada[10];
      bufferSalida[11]=bufferEntrada[11];
      bytesEnviados=bufferSalida[5]+6;
      server.write(bufferSalida,bytesEnviados);
      
      if (DEBUG) {
        Serial.print("direccion de la bobina a escribir: 0x");
        Serial.println(direccionBase,HEX);
        Serial.println("respuesta enviada: ");
        for (i=0;i<bytesEnviados;i++) {
          Serial.print(" 0x");
          Serial.print(bufferSalida[i],HEX);
          Serial.print("\n");
        } 
      }      
      break;
    case 6: //escribir un solo holding register que es un registro de 16 bits pensado para configuracion
    //programa ejemplo para enviar un comando de este tipo desde python
    //from pymodbus.client.sync import ModbusTcpClient
    //client=ModbusTcpClient('192.168.0.2')
    //response=client.write_register(12,0xabcd)
    //client.close()
    //    
      direccionBase=(unsigned int) bufferEntrada[8];
      direccionBase=direccionBase<<8;
      direccionBase=direccionBase|((unsigned int) bufferEntrada[9]);

      OutputValue=(unsigned int) bufferEntrada[10];
      OutputValue=OutputValue<<8;
      OutputValue=OutputValue|((unsigned int) bufferEntrada[11]);
      switch (direccionBase) {
      //aqui se debe añadir un caso por cada holding register que se pueda escribir en el sistema
        default:
            if (DEBUG) {
              Serial.print("el holding register ");
              Serial.print(direccionBase);
              Serial.print("se ha puesto a ");
              Serial.print(OutputValue,HEX);
              Serial.print("\n");
            }
      }
      //la cabecera se copia
      for (i=0;i<7;i++) bufferSalida[i]=bufferEntrada[i];
      bufferSalida[5]=6; //bytes que quedan en la respuesta
      //codigo de funcion
      bufferSalida[7]=0x06;
      //los siguientes dos bytes es la direccion del registro
      bufferSalida[8]=bufferEntrada[8];
      bufferSalida[9]=bufferEntrada[9];
      //los siguientes dos bytes es el nuevo valor
      bufferSalida[10]=bufferEntrada[10];
      bufferSalida[11]=bufferEntrada[11];
      bytesEnviados=bufferSalida[5]+6;
      server.write(bufferSalida,bytesEnviados);
      
      if (DEBUG) {
        Serial.print("direccion del registro a escribir: 0x");
        Serial.println(direccionBase,HEX);
        Serial.print("el nuevo valor del registro es: 0x");
        Serial.println(OutputValue,HEX);
        Serial.println("respuesta enviada: ");
        for (i=0;i<bytesEnviados;i++) {
          Serial.print(" 0x");
          Serial.print(bufferSalida[i],HEX);
          Serial.print("\n");
        } 
      }      
      break;
    case 15: //escribir multiples bobinas. De momento no se implementa. Se hará cuando la escritura de una bobina se haga en una funcion
    case 16: //escribir multiples registros. De momento no se implementa. Se hará cuando la escritura de un registro se haga en una funcion
    case 22: //no implementada pero quizás sea útil en algún caso concreto. Es para escribir un holding register con una mascara concreta de forma que solo algunos bits se sobreescriben
    case 23: //Lectura y escritura de multiples registros de una vez. No implementada. 
    case 43: //43 y 14 son lo mismo
    case 14: //No implementado aun. Es para pedir al dispositivo información sobre sí mismo como fabricante, Modelo, etc...    
    default:
      if (DEBUG) {
        Serial.println("comando no contemplado recibido");
        Serial.print("Numero de comando: ");
        Serial.print((int) bufferEntrada[6]);
        Serial.print("\n");
        Serial.println("Comando completo:");
        for (i=0;i<longitudComando;i++) {
          Serial.print(" 0x");
          Serial.print(bufferEntrada[i],HEX);
        }
        Serial.print("\n");
      }
  }
}
