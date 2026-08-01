Andreas Kromke
1. August 2026 A.D.

nderungen an der Ballerburg
============================

Bedienung
---------

- Men aufger„umt und fehlende "..." fr Meneintr„ge angeh„ngt.
- ^Q beendet das Programm
- Hilfe-Dialoge k”nnen mit [Return] weitergeschaltet werden
- Rechtschreibfehler in den Hilfe-Texten behoben.
- kosmetische nderung im Markt-Dialog
- Rechtschreibfehler im "About"-Dialog behoben.
- Fehlerdialog, wenn die Bildschirmaufl”sung nicht pat
- Warndialog, wenn man sein Geld verschleudert hat

Zur Problematik des verschleuderten Geldes:

Die Kapazit„t fr Kugeln, Gelds„cke und Pulver wird durch die Gr”e des 
Rechtecks fr die entsprechende Kammer in der jeweiligen Burg festgelegt. 
Leider kann man auf dem Markt auch dann Kugeln und Pulver kaufen, wenn die 
Kapazit„tsgrenze erreicht ist. Das Spiel nimmt dann einfach stillschweigend 
das Geld vom Spieler, aber die Ressourcen werden nicht erh”ht. Das ist 
„rgerlich. Dieser Fehler ist brigens auch in der SDL-Version. Eine Behebung 
war aufgrund der Komplexit„t des Programms nicht ad hoc m”glich, also habe ich 
Warndialoge eingebaut.


Sabotage von MagiC :-)
------------------

Das Programm lief nicht unter MagiC, und zwar lediglich deshalb, weil es die 
in den Bildspeicher greift und testet, ob die zweitoberste Bilschirmzeile 
schwarze Pixel enth„lt. Vermutlich wollte der Autor testen, ob ein Men 
heruntergefallen ist, was aber wegen BEG_MCTGRL nicht notwendig ist. Auerdem 
hat das Programm noch auf einen Timer gewartet, ihn aber nicht verwendet.

Links oben befindet sich das MagiC-Logo, und das hat die "event loop" 
blockiert.


Multitasking
------------

Ballerburg ”ffnet kein Fenster, schreibt ber VDI in den Bildschirm und 
verarbeitet keine REDRAW-Nachrichten. Es mu deshalb im "single application 
mode" ausgefhrt werden.

Um das zu korrigieren, mte man ein Fenster ”ffnen, dessen Inneres dem 
Zeichenbereich von etwa 620x400 Pixeln (unterhalb der Menleiste) entspricht, 
und das Programm massiv umbauen.


Bildschirm-Modi
---------------

Die originale Ballerburg greift direkt auf den Bildspeicher zu und 
funktioniert nur bei 640x400 monochrom. Ich habe diverse nderungen gemacht, 
so da das Programm unter MagiC jetzt in allen Aufl”sungen, sogar in True 
Colour, funktioniert. Dafr habe ich auch form_xdial() fr das Zeichnen und 
Entfernen der Dialoge verwendet, das unter TOS nicht existiert. Deshalb bleibt 
unter TOS die Beschr„nkung auf "ST High". Das k”nnte man relativ leicht 
„ndern, indem man form_xdial() per VDI nachbildet.

D.h. derzeit wird nur noch unter TOS direkt auf den Bildschirm zugegriffen und 
dieser kopiert und zurckgeschrieben (dafr gibt es einen Block von genau 
32000 Bytes). Unter MagiC wird dieser Speicherblock nicht angelegt.


Behobene Fehler
---------------

Ich habe einen fehlenden Parameter beim Funktionsaufruf erg„nzt und das 
šberschreiben eines Speicherblocks behoben. Das Programm hatte also nur 
zuf„llig funktioniert.

Beim Laden der Burg-Datei BALLER.DAT wurden keinerlei šberl„ufe erkannt.

Die Ausgabe des "pling" mu ber BIOS statt GEMDOS erfolgen, weil man bei 
GEMDOS-Ausgaben das Programm per ^C beenden kann.


Quelltext-nderungen
--------------------

Aus der "event loop" habe ich die o.g. "Sabotage" und den Timer entfernt.

Das Programm ist in K&R-C geschrieben, also nicht in ANSI-C. Ich mute deshalb 
alle Funktionen umsortieren und mit Protoypen versehen. Dabei habe ich der 
Einfachheit halber die beiden Quelltextdateien vereinigt.

Das Programm ist mit Lattice-C geschrieben, und ich habe es auf PureC 
portiert. Leider unterscheiden sich die Datentypen, so da ich alle "short" 
durch "int" ersetzen mute.

Die Variablen hatten krzestm”gliche Namen und die Funktioen sehr kurze. Weil 
PureC beim Compilieren und sp„testens beim Linken die Namen eh wegwirft und 
die L„nge keinen Einflu auf das sp„tere Programm hat, habe ich sie groteils 
umbenannt, damit man besser versteht, was sie sind und tun.

Der Quellext war optimiert fr einen kleinen Bildschirm, mit minimalen 
Leerzeichen und mehreren Operationen auf derselben Zeile. Das ging auf Kosten 
der Lesbarkeit, und ich habe das meiste umformatiert.

Das Programm hatte ein paar ambig”se Operationen, von denen PureC einige 
angemeckert hat, so da ich diverse Klammern hinzugefgt habe, um die korrekte 
Reihenfolge der Operation sicherzustellen.

Schlielich habe ich einen Haufen Kommentare hinzugefgt.


Abschluworte
-------------

Ich habe mich bewut NICHT in den "About"-Dialog eingetragen. Er ist bis auf 
die korrigierte Orthographie original.

Durch den ganzen Umbau k”nnten Fehler entstanden sein, insbesondere durch die 
Klammerung der Operationen.
