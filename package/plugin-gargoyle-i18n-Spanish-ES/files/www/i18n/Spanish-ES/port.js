/*
 * UTF-8 (with BOM) Spanish-ES text strings for port_forwarding(single,multi).sh html elements
 */

prtS.PISect="Redirección Individual de Puerto";
prtS.PRSect="Redirección de un Rango de Puertos";
prtS.ForIPort="Redireccionar Puertos Individuales desde WAN a LAN";
prtS.ForRPort="Redireccionar Rango de Puertos desde WAN a LAN";
prtS.DMZ="DMZ";
prtS.UseDMZ="Utilizar DMZ (Zona Desmilitarizada)";
prtS.DMZIP="IP de DMZ";
prtS.UP_NAT="UPnP / NAT-PMP";
prtS.UPNAT_En="Permitir UPnP &amp; servicio NAT-PMP";
prtS.APFor="Redirecciones de puertos activas";
prtS.USpd="Velocidad de subida a informar";
prtS.DSpd="Velocidad de descarga a informar";
prtS.UPHelp="UPnP (Universal Plug and Play) y NAT-PMP (NAT Port Mapping Protocol) son dos protocolos que permiten a los dispositivos y aplicaciones de la red LAN configurar automáticamente el enrutador con los redireccionamientos de puertos necesarios para funcionar correctamente. Si el dispositivo es compatible con cualquiera de los protocolos entonces no es necesario crear reglas para redireccionar puertos de forma manual (ver parte superior de esta página), ya que serán creadas automáticamente por el dispositivo.</p><p>Cuando esta función es activada Gargoyle muestra una tabla de redirecciones de puertos creadas automáticamente con el fin de que pueda observar cuales dispositivos han solicitado redirección y comprobar que esta característica esté funcionando correctamente. Este servicio puede no funcionar correctamente en las configuraciones de red que contienen dos o más enrutadores (NAT doble). Si observa que en la tabla existe una sola fila con '***' significa que no existen redirecciones de puertos registradas.</p><p>Como parte del protocolo el dispositivo de LAN puede solicitar la velocidad de la conexión WAN desde el enrutador. Se proporcionan dos campos para configurar la respuesta a tales solicitudes. Las aplicaciones cliente pueden utilizar esta información con el fin de optimizar su rendimiento. Pero es importante tener en cuenta que el enrutador no realiza ninguna acción para limitar las velocidades basándose en estos datos. Sólo se informa al solicitante. Si se introduce cero para cualquier valor, la velocidad de la interfaz es la informada, por lo general 100 MB o 1 GB, dependiendo de la velocidad de la interfaz del enrutador.</p><p>Existe cierta controversia sobre la seguridad de este servicio y además que requiere RAM adicional para ejecutarse lo cual puede llegar a ser importante en enrutadores con memoria reducida, por lo que esta función se encuentra desactivada por defecto.";

//templates
prtS.Desc="Descripción";
prtS.optl="(opcional)";
prtS.Proto="Protocolo";
prtS.FPrt="Desde Puerto";
prtS.TIP="Hacia IP";
prtS.TPrt="Hacia Puerto";
prtS.SPrt="Puerto Inicial";
prtS.EPrt="Puerto Final";

//javascript
prtS.AFRErr="No se pudo agregar la regla de redireccionamiento.";
prtS.GTErr="Puerto inicial > puerto final";
prtS.DupErr="El/los puerto(s) dentro del rango ya se encuentra(n) siendo redireccionado(s)";
prtS.CopErr="El puerto ya se encuentra siendo redireccionado";
prtS.UpErr="No se pudo actualizar el puerto redireccionado.";
prtS.Prot="Proto";
prtS.LHst="Equipo LAN";
prtS.Port="Puerto";

//edit.sh pages
prtS.PESect="Editar Redirección de Puerto";
