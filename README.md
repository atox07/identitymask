# Identity Mask 0.1 — GTA V

Menu ASI sperimentale per cambiare l'hash d'identita del modello del giocatore senza chiamare SET_PLAYER_MODEL e senza caricare un altro aspetto.

Esempio: il ped visibile e Franklin; scegli Michael; il modello gia caricato resta visibile, mentre GET_ENTITY_MODEL legge 0x0D7114C9.

## Stato e compatibilita

Questa e una prima implementazione sorgente: non e stata compilata o provata in GTA in questo ambiente. La build GitHub e il test nel gioco sono ancora necessari. Non e una versione gia collaudata del vecchio Character Swap.

Target iniziale: GTA V Legacy x64, modalita Storia, con ScriptHookV compatibile con la propria build e ASI Loader gia installati. Enhanced non verificato. L'hash viene modificato nel CBaseModelInfo (puntatore entita + 0x20, hash + 0x18): il codice verifica il valore rispetto alla native prima di scrivere e rifiuta layout non riconosciuti. Questo controllo riduce gli errori, ma non certifica la compatibilita di una build.

IMPORTANTE: CBaseModelInfo e condiviso. Altri ped dello stesso modello possono leggere l'hash modificato. La modifica non e isolata al singolo ped. I tre hash non trasformano ogni variabile, voce, abilita, statistica o script di missione nell'identita scelta: gli script che consultano altri dati possono comportarsi diversamente. Non viene garantita la compatibilita di missioni, cutscene, negozi, cambio protagonista e salvataggi. Non usare contemporaneamente altri editor di hash o Character Swap.

La maschera e solo nella RAM, non viene riapplicata all'avvio. Al cambio del puntatore ped/modello il plugin tenta di ripristinare il vecchio valore. Se un altro script cambia l'hash, il ripristino viene rifiutato per non sovrascriverlo. In caso di errore riavvia GTA. Prima di cambiare modello con un trainer, caricare un salvataggio, cambiare protagonista o ricaricare plugin, usa Ripristina. Non viene eseguita una scrittura durante DllMain allo scaricamento della DLL.

## Compilazione su GitHub

1. Crea un repository e carica il CONTENUTO di questa cartella nella radice: CMakeLists.txt, src, .github, IdentityMask.ini, README.md e LICENSE. Attenzione a includere la cartella nascosta .github.
2. Apri Actions → Build Identity Mask. Il push avvia la compilazione; puoi avviarla anche con Run workflow.
3. Al termine scarica l'artefatto IdentityMask-Windows-x64 e decomprimilo.
4. Copia IdentityMask.asi e IdentityMask.ini accanto a GTA5.exe.

La build usa CMake e MSVC x64 su windows-2022. Il plugin risolve dinamicamente le funzioni di ScriptHookV: nessun SDK, libreria .lib o file del gioco da caricare nel repository. ScriptHookV.dll rimane necessario nel gioco. Se mancano gli export richiesti il plugin non si carica.

## Comandi

| Comando | Tasto |
| --- | --- |
| Apri/chiudi | F8 |
| Seleziona | Freccia su/giu oppure Num8/Num2 |
| Applica | Invio oppure Num5 |
| Chiudi | Esc |

Nel menu trovi Michael, Franklin, Trevor e Ripristina identita originale. L'hash mostrato e quello letto in quel momento dalla native, non il nome dell'aspetto originale. Premi Ripristina per annullare la modifica. Il tasto apertura si cambia nel file INI con un codice virtual-key da F1 a F24 (F8=119, F9=120). Gli errori vengono scritti in IdentityMask.log accanto al plugin.

| Identita | Nome | Hash |
| --- | --- | --- |
| Michael | player_zero | 0x0D7114C9 |
| Franklin | player_one | 0x9B22DBAF |
| Trevor | player_two | 0x9B810FA2 |

## Prima prova in gioco

Conserva un salvataggio di prova. Avvia la Storia con Franklin, apri F8, scegli Michael: verifica che Franklin resti visibile e che il numero in menu diventi 0x0D7114C9. Usa Ripristina: il numero deve tornare 0x9B22DBAF. Ripeti con Trevor. Questo verifica il cambio hash, non tutte le missioni o l'intero stato del protagonista. Se compare Layout non riconosciuto, non vengono tentati offset alternativi: fornisci versione/edizione GTA e log per adattare il codice.

## Riferimenti tecnici

- ScriptHookV ufficiale: https://www.dev-c.com/gtav/scripthookv/
- Dichiarazioni ABI consultate: https://github.com/JupiterKasparov/GTAV_YOS/blob/main/scripthookv.pas
- Strutture modello: https://github.com/cpast/SirenSetting_Limit_Remover/blob/master/ASI_lib/RageStructs.h
- Riferimenti CPed: https://github.com/citizenfx/fivem/blob/master/code/components/extra-natives-five/src/PedExtraNatives.cpp
- Native database: https://github.com/alloc8or/gta5-nativedb-data

Implementazione indipendente. Nessun binario, codice o asset del vecchio Character Swap e incluso.
