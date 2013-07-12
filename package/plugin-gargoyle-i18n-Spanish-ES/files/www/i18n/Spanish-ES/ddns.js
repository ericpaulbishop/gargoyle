/*
 * UTF-8 (with BOM) Spanish-ES text strings for ddns.sh html elements
 */

DyDNS.DYSect="Servicios DNS Dinámico";
DyDNS.AddDy="Añadir Servicio DNS Dinámico";
DyDNS.SvPro="Proveedor de Servicios";
DyDNS.ChItv="Intervalo de Comprobación";
DyDNS.FUItv="Fuerza Intervalo de Actualización";
DyDNS.AddDDNS="Añadir Servicio DNS";
DyDNS.HelpCI="<em>Intervalo de Comprobación</em> especifica la frecuencia con la que el router comprobará si su IP actual coincide con la que actualmente está asociada a su nombre de dominio. Esta comprobación se realiza sin necesidad de conectarse a su proveedor de servicio DNS dinámico, lo que significa que esto no va a causar problemas con los proveedores que prohíben los usuarios que se conectan con demasiada frecuencia (p.ej. dyndns.com). Sin embargo, se establece una conexión de red para llevar a cabo esta comprobación, por lo que este valor no debe ser demasiado baja. Un intervalo de comprobación de entre 10 y 20 minutos es generalmente apropiada.";
DyDNS.HelpFI="<em>Fuerza Intervalo de Actualización</em> especifica la frecuencia con la que el router se conectará a su proveedor de servicio DNS dinámico y actualizar sus registros, incluso si su IP no ha cambiado. Los proveedores de servicios prohibir a los usuarios que actualizan con demasiada frecuencia, pero pueden cerrar las cuentas de usuarios que no se actualizan desde hace más de un mes. Se recomienda que se establezca este parámetro entre 3 y 7 días.";
DyDNS.UpErr1="Actualización de la nueva configuración del servicio de DNS dinámico(s) ha fallado";
DyDNS.UpErr2="Servicio(s) no se pudo actualizar correctamente, por lo que se han eliminado.";
DyDNS.cNams=["Dominio", "Última Actualización", "Activado", "", "" ];
DyDNS.InvErr="ERROR: proveedor especificado no es válido";
DyDNS.DupErr="Duplicar actualizar especificado.";
DyDNS.ForceU="Forzar actualización";
DyDNS.ModErr="Este servicio ha sido añadido/modificado y por lo tanto usted debe guardar sus cambios antes de una actualización se puede realizar. Haga clic \""+UI.SaveChanges+"\" e intente de nuevo.";
DyDNS.UpFErr="Ha fallado la actualización. Asegúrese de que su configuración es válida y que está conectado a Internet.";
DyDNS.UpOK="Actualización exitoso.";
DyDNS.UpSrvErr="No se pudo actualizar la clase de servicio.";

//ddns_edit.sh
DyDNS.EDSect="Editar Servicios DNS Dinámico";

// /etc/ddns_providers.conf
DyDNS.DoNm="Nombe de Dominio";
DyDNS.UsrN="Nombre de Usuario";
DyDNS.Pssw="Contraseña";
DyDNS.Eml="E-mail";
DyDNS.Key="Llave";
DyDNS.AKey="API Llave";
DyDNS.Tokn="Ficha";
