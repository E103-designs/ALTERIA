# Riferimenti e perimetro

Consultati il 2026-10-27:

- [Erica Synths: sorgente ufficiale LXR desktop](https://github.com/erica-synths/lxr-desktop-app). Il README descrive il target STM32F765 Cortex-M7 216 MHz e la memoria, non comparabili direttamente con il Nano ATmega328P.
- [DrumVoice.c](https://github.com/erica-synths/lxr-desktop-app/blob/main/Core/Src/Soundengine/DSPAudio/DrumVoice.c): famiglia drum con oscillatore, modulatore e inviluppi.
- [Snare.c](https://github.com/erica-synths/lxr-desktop-app/blob/main/Core/Src/Soundengine/DSPAudio/Snare.c): famiglia sottrattiva a componente tonale e rumore.
- [CymbalVoice.c](https://github.com/erica-synths/lxr-desktop-app/blob/main/Core/Src/Soundengine/DSPAudio/CymbalVoice.c): famiglia metallica con due modulatori.
- [HiHat.c](https://github.com/erica-synths/lxr-desktop-app/blob/main/Core/Src/Soundengine/DSPAudio/HiHat.c): famiglia hat, con controllo delle code.
- [Pagina prodotto e manuale ufficiale](https://www.ericasynths.lv/drum-synthesizer-lxr-02-1132/). Il link corrente di download restituisce un PDF come application/octet-stream; non sono state incorporate pagine o immagini del manuale.

DRUM LAB e **un'implementazione originale AVR dei concetti generali**, non una compilazione o conversione del codice DSP ufficiale. Nessuna funzione del repository LXR, preset, campione o asset grafico ufficiale e incorporata nel firmware. Sintesi e macro sono ridisegnate per due voci, 16 MHz e 2 KB SRAM. Driver DAC, tabella seno e procedura di calibrazione provengono dal progetto ALTERIA, Allrounder 2.0.2, mantenuto separato.

Il progetto ufficiale pubblica il proprio codice sotto GPL-3.0 con i termini aggiuntivi indicati nel suo NOTICE; questo documento non lo rilascia sotto una licenza diversa. Non confondere con la licenza del precedente repository SonicPotions/LXR o con firmware community diversi.

Le prime tre voci drum dell'LXR appartengono alla stessa famiglia generale: kick/tom/synth perc qui sono tre declinazioni progettate per ALTERIA, non tre copie dei rispettivi slot LXR. Snare e clap sono due declinazioni sottrattive; FM metal e gli hat rappresentano le altre due famiglie. Questa e la portata dell'espressione "tutte le famiglie", non una promessa di tutti gli algoritmi/parametri dell'originale o di identita sonora.

Erica Synths, Sonic Potions e LXR sono nomi dei rispettivi titolari, usati soltanto come riferimenti. Questo firmware non e affiliato, certificato o supportato da tali produttori e **non va caricato su un LXR-02**.
