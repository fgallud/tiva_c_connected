import re

velocidad_xy=200 #[mm/minuto]
velocidad_z=10  #[mm/minuto]
fichero_nombres=['agujeros_broca0.6.gcode',
                 'agujeros_broca0.8.gcode',
                 'agujeros_broca1.1.gcode']

def procesa_fichero(fichero_nombre):
    fichero=open(fichero_nombre,'r')
    texto_todo=fichero.read()
    fichero.close()
    texto_lineas=texto_todo.split('\n')
    texto_salida=''
    for linea in texto_lineas:
        if re.search('G0[0,1] X',linea):
            texto_salida+=linea+'F%i\n' %velocidad_xy
        elif re.search('G0[0,1] Z',linea):
            texto_salida+=linea+'F%i\n' %velocidad_z
        else:
            texto_salida+=linea+'\n'
    fichero=open(fichero_nombre+'procesado','w')
    fichero.write(texto_salida)
    fichero.close()

for fichero_nombre in fichero_nombres:
    procesa_fichero(fichero_nombre)



