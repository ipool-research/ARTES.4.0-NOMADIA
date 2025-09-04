#include "global.h"
#include "WAVFileWriter.h"

/* Definizione variabili  
--------------------------------------------------------*/
int wav_file_size;            // Dimensione corrente del file WAV (in byte)
wav_header_t wav_file_header; // Struttura per l'intestazione del file WAV (header RIFF/WAV)
FILE *wav_file;               // Puntatore al file WAV aperto

/* Definizione prototipi 
---------------------------------------------------------*/
esp_err_t WAVWRITERinit(void)
{
    return SDCARDinit();
}

esp_err_t WAVWRITERcreateFile(char *file_name, int sample_rate)
{
    esp_err_t ret;
    uint16_t numBytesWrite;

    // Inizializza i campi dell'header WAV per formato PCM mono 16-bit
    wav_file_header.wav_size = 0;
    wav_file_header.fmt_chunk_size = 16;
    wav_file_header.audio_format = 1;
    wav_file_header.num_channels = 1;
    wav_file_header.sample_rate = sample_rate;
    wav_file_header.bit_depth = 16;
    wav_file_header.byte_rate = (wav_file_header.sample_rate * wav_file_header.num_channels * wav_file_header.bit_depth / 8);
    wav_file_header.sample_alignment = (wav_file_header.num_channels * wav_file_header.bit_depth / 8);
    wav_file_header.data_bytes = 0;

    // Imposta intestazioni testuali "RIFF", "WAVE", "fmt " e "data"
    wav_file_header.riff_header[0] = 'R';
    wav_file_header.riff_header[1] = 'I';
    wav_file_header.riff_header[2] = 'F';
    wav_file_header.riff_header[3] = 'F';
    wav_file_header.wave_header[0] = 'W';
    wav_file_header.wave_header[1] = 'A';
    wav_file_header.wave_header[2] = 'V';
    wav_file_header.wave_header[3] = 'E';
    wav_file_header.fmt_header[0] = 'f';
    wav_file_header.fmt_header[1] = 'm';
    wav_file_header.fmt_header[2] = 't';
    wav_file_header.fmt_header[3] = ' ';
    wav_file_header.data_header[0] = 'd';
    wav_file_header.data_header[1] = 'a';
    wav_file_header.data_header[2] = 't';
    wav_file_header.data_header[3] = 'a';

    char path[32];
    sprintf(path, "%s/%s", MOUNT_POINT, file_name);
    printf("Creating file %s: ", path);

    // Crea il file WAV sulla SD card in modalità scrittura binaria
    wav_file = fopen(path, "wb");
    if (wav_file == NULL) {
        printf("error\r\n");
        return ESP_FAIL;
    }
    printf("ok\r\n");

    fwrite(&wav_file_header, sizeof(wav_header_t), 1, wav_file);  // Scrive l'header WAV iniziale nel file (verrà aggiornato alla chiusura)
    wav_file_size = sizeof(wav_header_t);                         // Imposta la dimensione iniziale del file (solo header)

    return ESP_OK;
}

esp_err_t WAVWRITERwriteFile(int16_t *samples, int count)
{
    // Scrive i campioni audio e aggiorna la dimensione del file
    fwrite(samples, sizeof(int16_t), count, wav_file);
    wav_file_size += sizeof(int16_t) * count;
    // printf("file write\r\n");
    return ESP_OK;
}

esp_err_t WAVWRITERcloseFile(void)
{
    // Aggiorna l'header WAV con le informazioni finali e lo riscrive su file
    wav_file_header.data_bytes = wav_file_size - sizeof(wav_header_t);  // Numero di byte di dati audio effettivi
    wav_file_header.wav_size = wav_file_size - 8;                       // Dimensione totale file - 8 (RIFF header non conta questi 8 byte)
    fseek(wav_file, 0, SEEK_SET);                                      // Torna all'inizio del file
    fwrite(&wav_file_header, sizeof(wav_header_t), 1, wav_file);        // Sovrascrive l'intestazione con i valori aggiornati
    fclose(wav_file);
    printf("file closed\r\n");
    return ESP_OK;
}
