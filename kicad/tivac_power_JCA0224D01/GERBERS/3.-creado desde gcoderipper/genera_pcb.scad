/*
Genera el DXF necesario para procesar un PCB con laser
El laser elimina una capa protectora antiacido de las zonas en que se quiere eliminar el cobre
el flujo de trabajo es:
- se disena en kicad y se exporta:
  - el archivo de taladros
  - el borde del pcb a dxf llamado edge.dxf
  - la capa FRONT a gerber
  - la capa BACK a gerber
- en FlatCAM se generan las rutas para aislar las rutas a gcode
- en gcoderipper se exporta a dxf
- en freecad se eliminan las rutas de movimiento del cabezal sin fresar
- aqui se eliminan las rutas que han caido fuera del PCB, eso permite alinear bien el PCB en el laser

para ejecutar este fichero se abre, F6, exportar DXF o bien por linea de comandos
openscad -o salida_para_laser.dxf genera_pcb.scad
*/

translate([1,0,0]) difference() {
    hull() import(file = "edge.dxf");
    import(file="front.dxf");
}

rotate([0,180,0]) translate([1,0,0]) difference() {
    hull() import(file = "edge.dxf");
    import(file="back.dxf");
}



