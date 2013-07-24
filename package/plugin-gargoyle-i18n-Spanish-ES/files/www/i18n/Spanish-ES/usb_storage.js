/*
 * UTF-8 (with BOM) Spanish-ES text strings for USB Storage html elements
 */

usbSStr.SDisk="Discos compartidos";
usbSStr.Nomdsk="No se detectaron discos USB montados";
usbSStr.WFTP="Permitir el acceso a FTP desde WAN";
usbSStr.WpFTP="Permitir FTP pasivo desde WAN en los puertos";
usbSStr.CFWkg="Grupo de trabajo de CIFS";
usbSStr.CFUsr="Usuarios CIFS / FTP";
usbSStr.NewU="Nuevo usuario";
usbSStr.AddU="Agregar usuario";
usbSStr.Pasw="Contraseña";
usbSStr.CfPass="Confirmar contraseña";
usbSStr.ADir="Agregar disco / directorio compartido";
usbSStr.ADsk="Agregar disco compartido";
usbSStr.CShare="Discos compartidos actualmente";
usbSStr.Umnt="Desmontar";
usbSStr.UmntB="Desmontar todos los discos USB";
usbSStr.UmntWarn="Los discos USB deben ser desmontados antes de ser extraídos del enrutador. Los discos USB que se encuentren conectados serán montados automáticamente en el siguiente reinicio del enrutador.";
usbSStr.FDisk="Formatear discos";
usbSStr.NoUmntDev="<p>No se detectaron unidades conectadas y desmontadas.</p><p>Debe desmontar las unidades antes de intentar darles formato.</p>";
usbSStr.FmtWarn="AVISO: Formatear un disco borrará de forma permanente todo el contenido de ese disco.<p>El disco será formateado para almacenamiento con el sistema de archivos EXT4<br/>EXT4 puede no ser legible si la unidad USB es extraída y conectada luego a un equipo con Windows/Mac</p>";
usbSStr.DskForm="Disco a formatear";
usbSStr.PSwap="Porcentaje de intercambio (Swap)";
usbSStr.PStor="Porcentaje de almacenamiento";
usbSStr.MExtr="Utilizar disco como raíz externa (extroot)";
usbSStr.FmtNow="Formatear ahora";
usbSStr.ExtrS="Extroot";
usbSStr.ExtrOff="Desactivar raíz externa";
usbSStr.ExtDt="Raíz externa detectada en";
usbSStr.ExtrWarn="Cuando se utiliza una unidad raíz externa o <strong>extroot</strong> una unidad USB externa se convierte en la unidad raíz de su enrutador. Esto permite establecer una unidad raíz de gran capacidad pero si se quitara la unidad entonces todos los cambios en la configuración desde que el extroot fuera activado se perderán.";

//template
usbSStr.Disk="Disco";
usbSStr.SDir="Subdirectorio";
usbSStr.SNam="Nombre del recurso compartido";
usbSStr.SAppl="Los ajustes se aplican a";
usbSStr.SPart="La misma partición en cualquier unidad USB";
usbSStr.SDriv="Sólo esta unidad específica";
usbSStr.STyp="Tipo(s) de recurso(s) compartido(s)";
usbSStr.FAAcc="Acceso anónimo FTP/CIFS";
usbSStr.ANon="Ninguno";
usbSStr.AROn="Sólo lectura";
usbSStr.ARWr="Lectura/Escritura";
usbSStr.FAUsr="Usuarios con acceso FTP/CIFS";
usbSStr.NAcc="Acceso a NFS";
usbSStr.NAccPo="Política de acceso a NFS";
usbSStr.AnonAcc="Permitir acceso anónimo";
usbSStr.OnlyIPs="Permitir sólo las siguientes IPs";
usbSStr.IPSub="Especifique una IP o subred";
usbSStr.FPath="Ruta FTP";
usbSStr.NPath="Ruta NFS";

//edit files
usbSStr.EshDS="Editar disco compartido";
usbSStr.ChUPass="Cambiar contraseña de usuario";
usbSStr.User="Usuario";
usbSStr.NPass="Nueva contraseña";

//javascript
usbSStr.ICWErr="Grupo de trabajo de CIFS inválido";
usbSStr.FWPErr="No se puede habilitar el acceso FTP desde WAN debido a que el puerto se encuentra en conflicto con";
usbSStr.FprErr="Rango inválido de puertos para FTP Pasivo";
usbSStr.FprcErr="El rango de puertos para FTP pasivo se encuentra en conflicto con";
usbSStr.SSetErr="No se pudo guardar la configuración";
usbSStr.UsEmErr="El nombre de usuario no puede estar vacío";
usbSStr.UsChErr="El nombre de usuario sólo puede contener letras y números";
usbSStr.PwEmErr="La contraseña no puede estar vacía";
usbSStr.PwEqErr="Las contraseñas no coinciden";
usbSStr.PwUErr="La contraseña no ha cambiado.";
usbSStr.UsDErr="Nombre de usuario duplicado";
usbSStr.Usnm="Nombre de usuario";
usbSStr.RsvErr="está reservado y no se permite";
usbSStr.AUsErr="No se pudo añadir el usuario";
usbSStr.AShErr="No se pudo agregar el recurso compartido";
usbSStr.NUsrErr="No hay usuarios definidos para los recursos compartidos";
usbSStr.NShUsrErr="No hay usuario para añadir -- Debe configurar un nuevo usuario más arriba.";
usbSStr.NoShTypErr="Debe seleccionar al menos un tipo de recurso compartido (FTP, CIFS, NFS)";
usbSStr.NoAAUsrErr="Se ha seleccionado FTP y/o CIFS pero el acceso anónimo o los usuarios no se encuentran configurados";
usbSStr.DupDirErr="El directorio compartido especificado ya se encuentra configurado";
usbSStr.DupShrErr="El nombre del recurso compartido es un duplicado";
usbSStr.UpShrErr="No se pudo actualizar el recurso compartido.";
usbSStr.SwpPErr="ERROR: Los porcentajes de asignación especificados no son válidos";
usbSStr.InvPErr="ERROR: Contraseña no válida";
usbSStr.ChPass="Cambiar contraseña";
usbSStr.Name="Nombre";
usbSStr.UDisk="Desmontaje de discos";
usbSStr.FDmsg1="¿Está seguro que desea formatear la unidad";
usbSStr.FDmsg2="Se perderán todos los datos de esta unidad.";
usbSStr.FDmsg3="Para continuar, introduzca la contraseña de inicio de sesión del enrutador";
usbSStr.CfmPass1="Contraseña aceptada.";
usbSStr.CfmPass2="Esta es su ÚLTIMA OPORTUNIDAD para cancelar. Pulse 'OK' sólo si está seguro de que quiere formatear";
usbSStr.FmtMsg="Formateando,\nPor favor, espere...";
usbSStr.ExtOffWarn="Si desactiva extroot todos los ajustes y plugins instalados se pueden perder. Después de desactivar extroot, el enrutador requiere ser reiniciado. ¿Continuar?";
usbSStr.ExtOffMsg="Desactivando extroot...";
usbSStr.SRboot="El sistema se está reiniciando";
