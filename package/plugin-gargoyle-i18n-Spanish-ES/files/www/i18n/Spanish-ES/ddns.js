/*
 * UTF-8 (with BOM) Spanish-ES text strings for ddns.sh html elements
 */

DyDNS.DYSect="Servicios de DNS Dinámico";
DyDNS.AddDy="Agregar Servicio de DNS Dinámico";
DyDNS.SvPro="Proveedor de Servicio";
DyDNS.ChItv="Intervalo de Comprobación";
DyDNS.FUItv="Intervalo de Actualización Forzada";
DyDNS.AddDDNS="Agregar Servicio DDNS";
DyDNS.HelpCI="<em>Intervalo de Comprobación</em> especifica la frecuencia con la que el enrutador comprobará si su IP actual coincide con la que actualmente está asociada a su nombre de dominio. Esta comprobación se realiza sin necesidad de conectarse a su proveedor de servicio de DNS dinámico, lo que significa que esto no causará problemas con los proveedores que prohiben a los usuarios que se conectan con demasiada frecuencia (por ejemplo, dyndns.com). Sin embargo, se establece una conexión de red para llevar a cabo esta comprobación, por lo que este valor no debe ser demasiado bajo. Un intervalo de comprobación de entre 10 y 20 minutos es generalmente apropiado.";
DyDNS.HelpFI="<em>Intervalo de Actualización Forzada</em> especifica la frecuencia con la que el enrutador se conectará a su proveedor de servicio de DNS dinámico y actualizará sus registros, incluso si su IP no ha cambiado. Los proveedores de servicios prohiben a los usuarios que actualizan con demasiada frecuencia aunque, por otro lado, pueden llegar a cerrar las cuentas de usuarios que no se actualicen por un mes entero. Se recomienda que establezca este parámetro entre 3 y 7 días.";
DyDNS.UpErr1="La actualización de la(s) nueva(s) configuración(es) del servicio de DNS dinámico ha fallado";
DyDNS.UpErr2="Los servicios no han podido ser actualizados correctamente y por lo tanto han sido eliminados.";
DyDNS.cNams=["Dominio", "Última Actualización", "Activado", "", "" ];
DyDNS.InvErr="ERROR: El proveedor especificado no es válido";
DyDNS.DupErr="Se ha especificado una actualización duplicada.";
DyDNS.ForceU="Forzar actualización";
DyDNS.ModErr="Este servicio ha sido agregado/modificado y por lo tanto debe guardar sus cambios antes de que una actualización pueda ser realizada. Haga clic en\""+UI.SaveChanges+"\" e intente nuevamente.";
DyDNS.UpFErr="Ha fallado la actualización. Asegúrese que su configuración es válida y que se encuentra conectado a Internet.";
DyDNS.UpOK="Actualización exitosa.";
DyDNS.UpSrvErr="No se ha podido actualizar la clase del servicio.";

//ddns_edit.sh
DyDNS.EDSect="Editar Servicio de DNS Dinámico";

// /etc/ddns_providers.conf
DyDNS.DoNm="Nombre de Dominio";
DyDNS.UsrN="Nombre de Usuario";
DyDNS.Pssw="Contraseña";
DyDNS.Eml="E-mail";
DyDNS.Key="Clave";
DyDNS.AKey="Clave de API";
DyDNS.Tokn="Token";
