/*
 * UTF-8 (with BOM) Spanish-ES text strings for qos_(upload|download|edit_class|edit_rule).sh html elements
 */

qosStr.URSection="QoS (Cargar) -- Reglas de Clasificación";
qosStr.DRSection="QoS (Descargar) -- Reglas de Clasificación";
qosStr.UCSection="QoS (Cargar) -- Clases de Servicios";
qosStr.DCSection="QoS (Descargar) -- Clases de Servicios";
qosStr.DACCSect="QoS (Descargar) -- Control de la Congestión Activo";
qosStr.UEnable="Habilitar Calidad de Servicio (Cargar Dirección)";
qosStr.DEnable="Habilitar Calidad de Servicio (Descargar Dirección)";
qosStr.UTotBand="Total (Cargar) Ancho de Banda";
qosStr.DTotBand="Total Descargar Ancho de Banda";

qosStr.USCAbout="Cada clase de servicio de carga se especifica mediante tres parámetros: porcentaje de ancho de banda a la capacidad, ancho de banda mínimo y máximo ancho de banda.";
qosStr.DSCAbout="Cada clase de servicio se especifica por cuatro parámetros: porcentaje de ancho de banda a la capacidad, ancho de banda en tiempo real y el máximo ancho de banda y la bandera tiempo de ida y vuelta minimimze.";
qosStr.UMinBandAbout="<em>Ancho de Banda Mínimo</em> especifica los servicios mínimos esta clase se asignará cuando el enlace se encuentra en la capacidad. Para ciertas aplicaciones como VoIP o juegos en línea es mejor para especificar un servicio mínimo en bps en lugar de un porcentaje. QoS se satisfiy los servicios mínimos de todas las clases antes de la asignación del servicio restante a otras clases de espera.";
qosStr.DMinBandAbout="<em>Ancho de Banda Mínimo</em> especifica los servicios mínimos esta clase se asignará cuando el enlace se encuentra en la capacidad. Las clases que especifican servicios mínimos se denominan clases en tiempo real por el controlador de la congestión activa. Streaming de vídeo, VoIP y juegos en línea interactivos son ejemplos de aplicaciones que deben tener un ancho de banda mínimo para funcionar. Para determinar qué introducir el uso de la aplicación en una red LAN sin carga y observar la cantidad de ancho de banda que utiliza. A continuación, introduzca un número ligeramente superior a esta en este campo. QoS se satisfiy los servicios mínimos de todas las clases de primero antes de asignar a otras clases de espera así que tenga cuidado al utilizar anchos de banda mínimos moderación.";
qosStr.UTotBandAbout="<em>Total Banda de Ancho a Cargar</em> se debe establecer en torno al 98% de su ancho de banda de subida disponible. Introducción de un número que es demasiado alto dará lugar a QoS no cumplir con sus requisitos de clase. Introducción de un número que es demasiado baja será innecesariamente penalizar la velocidad de subida. Si está utilizando una conexión PPPoE compruebe la página web de su módem y utilizar su velocidad de la línea de enlace ascendente como el ancho de banda de subida. Otros tipos de conexiones deben utilizar un programa de prueba de velocidad (con QoS off) para determinar el ancho de banda de subida disponible. Tenga en cuenta que el ancho de banda se especifica en kbps. Hay 8 kilobits por kilobyte.";
qosStr.DTotBandAbout="Especificar <em>Total Descargar Bandwidth</em> correctamente es fundamental para lograr que la calidad de servicio. Si está utilizando la congestión activa, entonces acaba de establecer esto a la velocidad máxima de descarga ISP entregará. Si está utilizando una conexión PPPoE comprobar la página web de su módem y ponga esto en su velocidad DSL descendente.";
qosStr.PerBandAboutU="<em>Porcentaje Ancho de Banda a Capacidad</em> es el porcentaje de la anchura de banda total disponible que debe asignarse a esta clase cuando se utiliza todo el ancho de banda disponible. Si el ancho de banda no utilizado está disponible, más puede (y será) asignado. Los porcentajes pueden ser configurados para ser igual a más (o menos) de 100, pero cuando se aplican los ajustes de los porcentajes se ajustarán proporcionalmente, de forma que se suman a 100.";
qosStr.PerBandAboutD="<em>Porcentaje Ancho de Banda a Capacidad</em> es el porcentaje de la anchura de banda total disponible que debe asignarse a esta clase cuando se utiliza todo el ancho de banda disponible. Si el ancho de banda no utilizado está disponible, más puede (y será) asignado. Los porcentajes pueden ser configurados para ser igual a más (o menos) de 100, pero cuando se aplican los ajustes de los porcentajes se ajustarán proporcionalmente, de forma que se suman a 100. Este ajuste sólo entra en vigor cuando el enlace WAN está saturada.";
qosStr.RTTAbout="<em>Minimizar RTT</em> indica que el controlador activo congestión que se desea minimizar los tiempos de ida y vuelta (RTT), cuando esta clase es activa. Utilice esta configuración para los juegos en línea o aplicaciones de VoIP que necesitan bajos tiempos de ida y vuelta (tiempos de ping). Minimizar el RTT se produce a expensas de eficiente rendimiento de la WAN por lo que mientras estas clases están activos el rendimiento WAN disminuirá (por lo general alrededor del 20%).";
qosStr.MinSpeedWarn="Si usted no está utilizando el ACC, entonces debe establecer cuál es la velocidad mínima de su ISP entregará es y luego establece este número a la cantidad. En ISPs generales no proporcionan un ancho de banda mínimo garantizado, por lo que requerirá un poco de experimentación y la frustración de su parte para llegar a un número. Un método consiste en comenzar con un número que es la mitad de lo que usted piensa que debería ser, y luego probar su enlace a plena carga y hacer que todo funcione. A continuación, aumentar en etapas, las pruebas a medida que avanza hasta QoS empieza a descomponerse. También puede ver que después de sus pruebas QoS funciona por un tiempo y luego deja de funcionar. Esto se debe a que su ISP se está sobrecargado debido a las demandas de sus otros clientes para que ya no están dando para el ancho de banda que hicieron durante su prueba. que la solución, bajar este número. Introducción de un número que es demasiado alto dará lugar a QoS no cumplir con sus requisitos de clase. Introducción de un número que es demasiado baja será innecesariamente penalizar la velocidad de descarga. Debido a todas estas complicaciones que a recomendado utilizar el ACC cuando sea posible. Tenga en cuenta que el ancho de banda se especifica en kilobits/s. Hay 8000 bits por kilobytes.";
qosStr.QoSAbout="Calidad de Servicio (QoS o 'Quality of Service' en Inglés) proporciona una manera de controlar cómo se asigna ancho de banda disponible. Las conexiones se clasifican en diferentes &ldquo;clases de servicio&rdquo;, cada uno de los cuales se le asigna una parte del ancho de banda disponible. QoS debe aplicarse en los casos en que quiera dividir el ancho de banda disponible entre los requisitos de la competencia. Por ejemplo, si quiere que su teléfono VoIP funcione correctamente, mientras que la descarga de vídeos. Otro caso sería si usted quiere que sus torrentes de bits estrangulado volver cuando esté navegación en la web.";
qosStr.MaxBandAbout="<em>Ancho de Banda Máximo</em> especifica la cantidad máxima absoluta de ancho de banda de esta clase se le asignará en kbit / s. Incluso si el ancho de banda no utilizado está disponible, esta clase de servicio no se le permitirá utilizar más de esta cantidad de ancho de banda.";
qosStr.PackAbout="Los paquetes son probados contra de las reglas en el orden especificado - Reglas hacia la parte superior tienen prioridad. Tan pronto como un paquete coincide con una regla que está clasificado, y el resto de las normas se ignoran. El orden de las reglas puede ser alterado usando los controles de flecha.";
qosStr.DefServClassAbout="La <em>Clase de Servicio Predeterminada</em> especifica cómo se deben clasificar los paquetes que no coincidan con ninguna regla.";
qosStr.AbACC="<p>El control de la congestión activo (ACC) observa a su actividad de descarga y ajusta automáticamente el límite enlace de descarga para mantener un rendimiento adecuado QoS. ACC compensa automáticamente los cambios en su velocidad de descarga de ISP y la demanda de la red ajustando la velocidad del enlace a la velocidad más alta posible que se preservará la función QoS adecuada. El alcance efectivo de este control es entre el 15% y el 100% del ancho de banda de descarga total que ingresó anteriormente. </p> Mientras que el CAC no ajusta su velocidad de enlace de subida debe habilitar y configurar adecuadamente su carga QoS para que funcionar correctamente. </p> <em> Ping Target-</em> el segmento de red entre el router y el destino de ping es donde se controla la congestión. Mediante el control de los tiempos de ping de ida y vuelta a la congestión de destino se detecta. Por defecto ACC utiliza la puerta de enlace WAN como objetivo ping. Si usted sabe que la congestión en el enlace tendrá lugar en un segmento diferente, entonces se puede introducir un destino de ping alternativa. </p> <em> Manual Ping-Limit </em> Ronda tiempos de ping viaje se comparan contra el ping límites. ACC controla el límite de enlace para mantener tiempos de ping bajo el límite correspondiente. Por defecto Gargoyle selecciona automáticamente los límites de ping apropiados para usted basado en las velocidades de enlace que ha entrado. Si a usted le gustaría probar diferentes límites puede introducir manualmente una vez aquí. Entrando veces más dará lugar a límites más altos de ping, y menor tiempo de límites más cortos. Usted puede ver los límites ACC está utilizando en los corchetes [] junto al campo de ping a plazos.</p>";
qosStr.ServClass="Clase de Servicio Predeterminada";

qosStr.AddNewClassRule="Añadir Nueva Regla de Clasificación";
qosStr.AddNewServiceRule="Añadir Nueva Clase de Servicio";
qosStr.SrcIP="Origen IP";
qosStr.SrcPort="Puerto(s) de Origen";
qosStr.DstIP="Destino IP";
qosStr.DstPort="Puerto(s) de Destino";
qosStr.MaxPktLen="Longitud Máxima de Paquetes";
qosStr.MinPktLen="Longitud Mínima de Paquetes";
qosStr.TrProto="Protocolo de Transporte";
qosStr.Conreach="Bytes de Conexión Llegan";
qosStr.AppProto="Protocolo de Aplicación (Layer7)";
qosStr.SetClass="Establezca Clase de Servicio a";
qosStr.SrvClassName="Servicio de Nombres de Clase";
qosStr.PerBandCap="Porcentaje Ancho de Banda a Capacidad";
qosStr.BandMin="Ancho de Banda Mínimo";
qosStr.BandMinNo="Ancho de Banda Sin Mínimo";
qosStr.BandMax="Ancho de Banda Máximo";
qosStr.BandMaxNo="Ancho de Banda Sin Máximo";
qosStr.MinRTT="Minimizar Tiempos de Ida y Vuelta (RTT)";
qosStr.ActRTT="Minimizar RTT (tiempos de ping) cuando está activo";
qosStr.OptiWAN="Optimizar la Utilización de la WAN";
qosStr.ACCOn="Habilitar control de la congestión activo (descarga dirección)";
qosStr.ACC_Pt="Utilice destino de ping no estándar";
qosStr.ACC_con="Controlar manualmente destino de ping";
qosStr.ACC_Stat="Congestion Control Estado";
qosStr.ACC_L_Ck="Revise para ver si el destino de ping responderá";
qosStr.ACC_L_In="Estimar un límite de ping";
qosStr.ACC_L_Act="Control de congestión activa.";
qosStr.ACC_L_Min="Control de congestión activa, minRTT clase activa.";
qosStr.ACC_L_Id="No congestión, control de ralentí.";
qosStr.ACC_L_Dis="El controlador no está habilitada";
qosStr.ACC_L_Lim="El límite de ancho de banda de descarga cumplir actualmente.";
qosStr.ACC_L_Fr="El límite de ancho de banda de descarga justo aparente.";
qosStr.ACC_L_Ld="El tráfico actual en el enlace descendente.";
qosStr.ACC_L_pg="El tiempo de ida y vuelta del último ping.";
qosStr.ACC_L_Flt="El tiempo de ida y vuelta se filtró.";
qosStr.ACC_L_plim="El punto en el que el controlador actuará para mantener la equidad.";
qosStr.ACC_L_AC="Número de Clases Descargar con carga sobre 4kbps.";

//qos_edit_class.sh
qosStr.QESrvClass="Corregir QoS Class Service";
qosStr.QESrvName="Nombre de Servicio de Clase";

//qos_edit_rule.sh
qosStr.QERulClass="Corregir QoS Clasificación Regla";

//javascript
qosStr.MatchC="Criterios de Coincidencia";
qosStr.Classn="Clasificación";
qosStr.Src="Origen";
qosStr.SrcP="Puerto de Origen";
qosStr.Dst="Destino";
qosStr.DstP="Puerto de Destino";
qosStr.Connb="bytes de Conexión";
qosStr.APro="Protocolo de Aplicación";
qosStr.pBdW="Por Ciento BW";
qosStr.mBdW="Min BW";
qosStr.MBdW="Máx BW";
qosStr.qLd="Carga";
qosStr.CrErr="No hay criterios de coincidencia han seleccionado.";
qosStr.SvErr="No se pudo agregar nueva clase de servicio.";
qosStr.SUErr="No se pudo actualizar la clase de servicio.";
qosStr.CsErr="No se pudo agregar reglas de clasificación.";
qosStr.CUErr="No se pudo actualizar la norma de clasificación.";
qosStr.DCErr="Duplicar nombre de la clase.";
qosStr.RemSCErr="Se requiere al menos una clase de servicio.\nNo se pueden quitar clase de servicio.";
qosStr.TotErr="Hay un error en el campo Total de Ancho de Banda.\n\nNo se pudo actualizar QoS.";

//one-word strings used in rules
qosStr.NOLIMIT="sinlímite";
qosStr.ZERO="cero";
qosStr.YES="Sí";
