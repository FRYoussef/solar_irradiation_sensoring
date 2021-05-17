# Diario de pruebas de nodos

## 13 Mayo. Pruebas UCM

 * Nodo 1 enviaba negativos. Deja de hacerlo al quitarlo de la placa de expansión y separar bien los conectores del panelcito. Empieza a enviar positivos incluso conectado a placa de expansión.
* Mismo problema con nodo6. Ahora envía positivo tras desconectar y separar

## 14 Mayo. Pruebas UCM

* Nodos 1,5,6 durmieron toda la noche (de 20 a 9). Despiertan bien y siguen enviandoo positivos
* [9:20am] Pongo en marcha nodos 2 y 3 con batería cargada durante la noche. Envían positivos
* Las medidas del nodo3 parecen más bajas de lo que deberían —> Sólo era por su ubicación
* Nodo 1 y 2 parecen  ruidosos y va a más. Los cables del panel están pelados y puede haber corto.
  * Nodo 1 y 2 dan lecturas de batería de 8K !! (A veces). No parece haber correlación con valores muy altos del panecito.
  * [14:30] Separo cables de panel de nodos 1 y 2. Nodo 5 también se ha vuelto ruidoso desde las 13:55. Los cables parecen correctos.
* [15:30] nodo3 deja de enviar. No lo hace ni tras resetear
* Hay *derivas de minutos en los timestamps* y diferentes en cada nodo.

## 15- 16 de mayo. Pruebas UCM
 * Arrancan todos, pero con desviación horaria desde el inicio: *HAY QUE FORZAR SNTP SIEMPRE*
 * Todo el fin de semana envían y siguen el ciclo dormir/despertar. 
 * El nodo 3 nunca vuelve a la vida

## 17 de mayo. Pruebas UCM
  * Actualización de código (rama influxdb-format). Se fuerza sincronización SNTP cada hora
    * Se reduce mucho la deriva horario. Aún así, **los envíos cada 30 segundos no son exactos. Parece haber un efecto acumulativo en el desfase, pero a veces adelanta y a veces atrasan**
  * [13:00] nodo6 conectado a la placa de expansión. El resto no.
  * [16:33] nodo1 y nodo2 también conectados a placa de expansión. nodo4 y nodo5 no
 

