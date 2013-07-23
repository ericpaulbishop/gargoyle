/*
 * UTF-8 (with BOM) Spanish-ES text strings for qos_(upload|download|edit_class|edit_rule).sh html elements
 */

qosStr.URSection="QoS (Subida) -- Reglas de clasificación";
qosStr.DRSection="QoS (Descarga) -- Reglas de clasificación";
qosStr.UCSection="QoS (Subida) -- Clases de servicios";
qosStr.DCSection="QoS (Descarga) -- Clases de servicios";
qosStr.DACCSect="QoS (Descarga) -- Control Activo de Congestión";
qosStr.UEnable="Activar Calidad de Servicio (Dirección de subida)";
qosStr.DEnable="Activar Calidad de Servicio (Dirección de descarga)";
qosStr.UTotBand="Ancho de banda total (Subida)";
qosStr.DTotBand="Ancho de banda total (Descarga)";

qosStr.USCAbout="Cada clase de servicio de subida se especifica mediante tres parámetros: porcentaje de ancho de banda a la capacidad, ancho de banda mínimo y ancho de banda máximo.";
qosStr.DSCAbout="Cada clase de servicio se especifica mediante cuatro parámetros: porcentaje de ancho de banda a la capacidad, ancho de banda de tiempo real, ancho de banda máximo y la bandera de minimizar el tiempo de ida y vuelta.";
qosStr.UMinBandAbout="<em>Ancho de banda mínimo</em> especifica los servicios mínimos que esta clase asignará cuando el enlace se encuentre a esta capacidad. Para ciertas aplicaciones como VoIP o juegos en línea es mejor especificar un servicio mínimo en bps en lugar de un porcentaje. QoS satisfará los servicios mínimos de todas las clases antes de asignar el servicio restante a otras clases que se encuentren en espera.";
qosStr.DMinBandAbout="<em>Ancho de banda mínimo</em> especifica los servicios mínimos que esta clase asignará cuando el enlace se encuentra a esta capacidad. Las clases que especifican servicios mínimos se denominan clases en tiempo real por el Controlador Activo de Congestión. Streaming de vídeo, VoIP y juegos en línea interactivos son ejemplos de aplicaciones que deben tener un ancho de banda mínimo para funcionar. Para determinar qué valor introducir utilice la aplicación en una red LAN sin carga y observe la cantidad de ancho de banda que utiliza. A continuación, introduzca un número ligeramente superior al observado en este campo. QoS satisfará los servicios mínimos de todas las clases primero antes de asignar a otras clases en espera por lo que sea cuidadoso y utilice con moderación los anchos de banda mínimos.";
qosStr.UTotBandAbout="<em>Ancho de banda total de subida</em> se debe establecer en torno al 98% de su ancho de banda de subida disponible. El ingreso de un número demasiado alto dará lugar a que QoS no pueda cumplir con sus requerimientos de clase. El ingreso de un número demasiado bajo penalizará innecesariamente la velocidad de subida. Si está utilizando una conexión PPPoE compruebe en la página web de su módem y utilice su velocidad de enlace ascendente como el ancho de banda de subida. Otros tipos de conexiones deben utilizar un programa de prueba de velocidad (con QoS desactivado) para determinar el ancho de banda de subida disponible. Tenga en cuenta que el ancho de banda se especifica en kbps. Hay 8 kilobits por kilobyte.";
qosStr.DTotBandAbout="Especificar el <em>ancho de banda total de descarga</em> de forma correcta es fundamental para lograr que QoS funcione. Si está utilizando la congestión activa entonces entonces establezca este valor a la velocidad máxima de descarga que su ISP le provee. Si está utilizando una conexión PPPoE compruebe la página web de su módem e ingrese este valor como su velocidad de descarga de DSL.";
qosStr.PerBandAboutU="<em>Porcentaje de ancho de banda a la capacidad</em> es el porcentaje de la anchura de banda total disponible que debe asignarse a esta clase cuando se utiliza todo el ancho de banda disponible. Si el ancho de banda no utilizado se encuentra disponible, puede ser (y será) asignada una mayor cantidad. Los porcentajes pueden ser configurados para ser igual a más (o menos) de 100, pero cuando se aplican los ajustes los porcentajes serán ajustados proporcionalmente de forma que sumen 100.";
qosStr.PerBandAboutD="<em>Porcentaje de ancho de banda a la capacidad</em> es el porcentaje de la anchura de banda total disponible que debe asignarse a esta clase cuando se utiliza todo el ancho de banda disponible. Si el ancho de banda no utilizado se encuentra disponible, puede ser (y será) asignada una mayor cantidad. Los porcentajes pueden ser configurados para ser igual a más (o menos) de 100, pero cuando se aplican los ajustes los porcentajes serán ajustados proporcionalmente de forma que sumen 100. Este ajuste sólo entra en vigor cuando el enlace WAN se encuentra saturado.";
qosStr.RTTAbout="<em>Minimizar RTT</em> indica al Controlador Activo de Congestión que se desea minimizar los tiempos de ida y vuelta (RTT) cuando esta clase se encuentra activa. Utilice esta configuración para juegos en línea o aplicaciones de VoIP que necesiten bajos tiempos de ida y vuelta (tiempos de ping). Minimizar el RTT se produce a expensas de un rendimiento eficiente de la WAN por lo que mientras estas clases están activas el rendimiento de la WAN disminuirá (por lo general alrededor del 20%).";
qosStr.MinSpeedWarn="Si no se encuentra utilizando el ACC entonces debe establecer cuál es la velocidad mínima que su ISP le provee y luego establecer este número a dicha cantidad. Por lo general, los ISPs no proporcionan un ancho de banda mínimo garantizado por lo que requerirá un poco de experimentación y la frustración de su parte hasta llegar a un número. Un método consiste en comenzar con un número que sea la mitad de lo que piensa que debería ser y luego probar su enlace a plena carga y asegurarse de que todo funcione. A continuación, incremente el número en etapas realizando pruebas a medida que avanza hasta que QoS empiece a degradarse o funcionar de manera incorrecta. También puede observar que, después de sus pruebas, QoS funciona por un tiempo y luego deja de funcionar. Esto se debe a que su ISP se encuentra sobrecargado debido a las demandas de sus otros clientes por lo que el ISP ya no le brinda el ancho de banda que tuvo durante las pruebas. La solución es reducir este número. Ingresar un número demasiado alto dará lugar a que QoS no pueda cumplir con sus requisitos de clase. Ingresar un número demasiado bajo penalizará innecesariamente la velocidad de descarga. Debido a todas estas complicaciones se recomienda utilizar el ACC cuando sea posible. Tenga en cuenta que el ancho de banda se especifica en kilobits/s. Hay 8000 bits por kilobytes.";
qosStr.QoSAbout="Calidad de Servicio (QoS o 'Quality of Service' en inglés) proporciona una manera de controlar cómo se asigna el ancho de banda disponible. Las conexiones se clasifican en diferentes &ldquo;clases de servicio&rdquo;, cada una de las cuales tiene asignada una porción del ancho de banda disponible. QoS debe aplicarse en los casos en que quiera dividir el ancho de banda disponible entre los requerimientos que compiten por la misma. Por ejemplo, si quiere que su teléfono VoIP funcione correctamente mientras se encuentre descargando vídeos. Otro caso sería si usted desea que las descargas de sus torrents sean limitadas mientras se encuentre navegando por la Web.";
qosStr.MaxBandAbout="<em>Ancho de banda máximo</em> especifica la cantidad máxima absoluta de ancho de banda que será asignada a esta clase en kbit/s. Incluso si el ancho de banda no utilizado se encuentra disponible, esta clase de servicio no estará permitida a utilizar más de esta cantidad de ancho de banda.";
qosStr.PackAbout="Los paquetes son probados contra las reglas en el orden especificado -- tienen prioridad las reglas de la parte superior. Tan pronto como un paquete coincide con una regla éste es clasificado y el resto de las reglas se ignoran. El orden de las reglas puede ser alterado utilizando los controles de flechas.";
qosStr.DefServClassAbout="La <em>Clase de servicio por defecto</em> especifica cómo se deben clasificar los paquetes que no coincidan con ninguna regla.";
qosStr.AbACC="<p>El Control Activo de Congestión (ACC) observa su actividad de descarga y ajusta automáticamente el límite del enlace de descarga para mantener un rendimiento adecuado de QoS. ACC compensa automáticamente los cambios en su velocidad de descarga del ISP y la demanda desde la red ajustando la velocidad del enlace a la velocidad más alta posible, lo cual mantendrá un adecuado funcionamiento de QoS. El alcance efectivo de este control es entre el 15% y el 100% del ancho de banda de descarga total que ingresó anteriormente. </p> Mientras que ACC no ajusta su velocidad de enlace de subida debe habilitar y configurar adecuadamente QoS de Subida para que funcione correctamente. </p> <em>Destino del Ping-</em> El segmento de red entre el enrutador y el destino del ping es aquel donde se controla la congestión. La congestión se detecta mediante el control de los tiempos de ping de ida y vuelta. Por defecto ACC utiliza la puerta de enlace de la WAN como destino del ping. Si sabe que la congestión en el enlace tendrá lugar en un segmento diferente de la red entonces se puede ingresar un destino del ping alternativo. </p><em>Límite de Ping Manual-</em> Los tiempos de ping de ida y vuelta se comparan contra los límites de ping. ACC controla el límite del enlace para mantener tiempos de ping debajo del límite apropiado. Por defecto Gargoyle selecciona automáticamente los límites de ping apropiados basándose en las velocidades de enlace que ha ingresado anteriormente. Si desea probar diferentes límites puede ingresar manualmente un valor de tiempo aquí. Ingresando tiempos más largos producirá límites de ping más altos y tiempos menores producirán límites más pequeños. Puede observar los límites que ACC se encuentra utilizando en los corchetes [] junto al campo de límites de ping.</p>";
qosStr.ServClass="Clase de servicio por defecto";

qosStr.AddNewClassRule="Añadir nueva regla de clasificación";
qosStr.AddNewServiceRule="Añadir nueva clase de servicio";
qosStr.SrcIP="IP de origen";
qosStr.SrcPort="Puerto(s) de origen";
qosStr.DstIP="IP de destino";
qosStr.DstPort="Puerto(s) de destino";
qosStr.MaxPktLen="Longitud máxima de paquete";
qosStr.MinPktLen="Longitud mínima de paquete";
qosStr.TrProto="Protocolo de transporte";
qosStr.Conreach="Cantidad de bytes alcanzados por la conexión";
qosStr.AppProto="Protocolo de aplicación (Capa 7)";
qosStr.SetClass="Establecer clase de servicio a";
qosStr.SrvClassName="Nombre de clase de servicio";
qosStr.PerBandCap="Porcentaje de ancho de banda a la capacidad";
qosStr.BandMin="Ancho de banda mínimo";
qosStr.BandMinNo="Ancho de banda sin mínimo";
qosStr.BandMax="Ancho de banda máximo";
qosStr.BandMaxNo="Ancho de banda sin máximo";
qosStr.MinRTT="Minimizar tiempos de ida y vuelta (RTT)";
qosStr.ActRTT="Minimizar RTT (tiempos de ping) al estar activo";
qosStr.OptiWAN="Optimizar utilización de WAN";
qosStr.ACCOn="Habilitar Control Activo de Congestión (Dirección de descarga)";
qosStr.ACC_Pt="Utilizar destino de ping no estándar";
qosStr.ACC_con="Controlar manualmente destinos de ping";
qosStr.ACC_Stat="Estado del control de congestión";
qosStr.ACC_L_Ck="Comprobar si el destino del ping responderá.";
qosStr.ACC_L_In="Estimar un límite de ping.";
qosStr.ACC_L_Act="El control de congestión se encuentra activo.";
qosStr.ACC_L_Min="El control de congestión se encuentra activo, la clase minRTT se encuentra activa.";
qosStr.ACC_L_Id="Sin congestión, control en espera.";
qosStr.ACC_L_Dis="El controlador se encuentra deshabilitado.";
qosStr.ACC_L_Lim="Límite del ancho de banda de descarga a cumplir.";
qosStr.ACC_L_Fr="Límite del ancho de banda de descarga equitativo aparente.";
qosStr.ACC_L_Ld="Tráfico actual en el enlace descendente.";
qosStr.ACC_L_pg="Tiempo de ida y vuelta del último ping.";
qosStr.ACC_L_Flt="Tiempo de ida y vuelta filtrado.";
qosStr.ACC_L_plim="Punto en el que el controlador entrará en acción para mantener la equidad.";
qosStr.ACC_L_AC="Número de clases de descarga con una carga superior a 4 kbps.";

//qos_edit_class.sh
qosStr.QESrvClass="Editar clase de servicio QoS";
qosStr.QESrvName="Nombre de clase de servicio";

//qos_edit_rule.sh
qosStr.QERulClass="Editar regla de clasificación QoS";

//javascript
qosStr.MatchC="Criterios de coincidencia";
qosStr.Classn="Clasificación";
qosStr.Src="Origen";
qosStr.SrcP="Puerto de origen";
qosStr.Dst="Destino";
qosStr.DstP="Puerto de destino";
qosStr.Connb="Bytes de la conexión";
qosStr.APro="Protocolo de aplicación";
qosStr.pBdW="Porc. de AB";
qosStr.mBdW="AB mín";
qosStr.MBdW="AB máx";
qosStr.qLd="Carga";
qosStr.CrErr="No hay criterios de coincidencia seleccionados.";
qosStr.SvErr="No se pudo agregar la nueva clase de servicio.";
qosStr.SUErr="No se pudo actualizar la clase de servicio.";
qosStr.CsErr="No se pudo agregar la regla de clasificación.";
qosStr.CUErr="No se pudo actualizar la regla de clasificación.";
qosStr.DCErr="Nombre de clase duplicado.";
qosStr.RemSCErr="Se requiere al menos una clase de servicio.\nNo se puede quitar la clase de servicio.";
qosStr.TotErr="Hay un error en el campo Ancho de banda total.\n\nNo se pudo actualizar QoS.";

//one-word strings used in rules
qosStr.NOLIMIT="sin límite";
qosStr.ZERO="cero";
qosStr.YES="Sí";

//qos_distribution.sh
qosStr.UBSect="Distribución del ancho de banda para QoS de subida";
qosStr.DBSect="Distribución del ancho de banda para QoS de descarga";
qosStr.uTFrm="Intervalo de tiempo de subida";
qosStr.dTFrm="Intervalo de tiempo de descarga";
