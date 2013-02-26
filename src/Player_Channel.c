#include <stdlib.h>
#include <math.h>
#include "MOD.h"
#include "MOD_Channel.h"
#include "Player.h"
#include "utils.h"

#define PI 3.142592

MOD_Player_Channel* MOD_Player_Channel_create(int channel_number){
    MOD_Player_Channel* channel = (MOD_Player_Channel*) malloc(sizeof(MOD_Player_Channel));
    channel->sample_tracker = 0;
    channel->tick = 0;
    channel->sample_rate = 44100;
    channel->vibrato_waveform = WAVEFORM_SINE;
    channel->vibrato_amplitude = 0;
    channel->vibrato_period = 0;
    channel->vibrato_tick = 0;
    channel->volume = 64;
    channel->sample = NULL;
    channel->number = channel_number;
    return channel;
}

double MOD_Player_Channel_step(MOD_Player_Channel* player_channel, MOD_Player* player, MOD* mod){

    MOD_Channel* channel = mod->patterns[mod->pattern_table[player->song_position]]->divisions[player->active_division]->channels[player_channel->number];
    double sample_period = player_channel->sample_period;

    double modifier = pow(2, 2*player_channel->vibrato_amplitude*sin(
        player_channel->vibrato_period*player_channel->vibrato_tick*player->sample_rate/150000000.*PI
    )/12.);


    if(isnan(modifier)){
        modifier = 1;
    }

    sample_period /= modifier;



    double out;
    if(player_channel->sample != 0){
        MOD_Sample* sample = player_channel->sample;

        double thr = sample_period/(double)AMIGA_FREQUENCY*player_channel->sample_rate;

        while(player_channel->tick > thr){

            player_channel->tick -= thr;
            player_channel->sample_tracker++;

            if(sample->repeat_length*2 > 1){
                while(player_channel->sample_tracker >= sample->length*2){
                    player_channel->sample_tracker -= sample->repeat_length*2; 
                }
            }
        }

        if(player_channel->sample_tracker < sample->length*2){
            int di = ((int)player_channel->sample_tracker)%(sample->length*2);
            int current_byte = sample->data[di];
            out = current_byte * sample->volume*0.5/64. * player_channel->volume;
        }else{
            out = 0;
        }
    }else{
        out = 0;
    }

    player_channel->tick++;
    return out;
}


void MOD_Player_Channel_set_volume(MOD_Player_Channel* player_channel, double volume){
    volume = MAX(volume, 0);
    volume = MIN(volume, 64);
    player_channel->volume = volume;
}

void MOD_Player_Channel_process_effect(MOD_Player_Channel* player_channel, MOD_Player* player, int effect){

    int e = (effect&0xf00) >> 8;
    int x = (effect&0x0f0) >> 4;
    int y =  effect&0x00f;

    switch(e){

        case EFFECT_ARPEGGIO:
            /*
            ;int step = (int)(3*player->tick/player->ticks_per_division);
            if(step == 1){ sample_period /= pow(2, x/12.);}
            if(step == 2){ sample_period /= pow(2, y/12.);}
            */
            break;

        case EFFECT_SLIDE_UP:
            break;
        case EFFECT_SLIDE_DOWN:
            break;
        case EFFECT_SLIDE_TO_NOTE:
            break;
        case EFFECT_VIBRATO:
            /*
            player_channel->vibrato_waveform = WAVEFORM_SINE;
            player_channel->vibrato_amplitude = y/16.;
            player_channel->vibrato_period = (x*player->ticks_per_division)/64.;
            player_channel->vibrato_tick++;
            */
            break;
        case EFFECT_CONTINUE_SLIDE_TO_NOTE_AND_VOLUME_SLIDE:
            MOD_Player_Channel_set_volume(player_channel, player_channel->volume + x == 0 ? -y : x);
            break;
        case EFFECT_CONTINUE_VIBRATO_TO_NOTE_AND_VOLUME_SLIDE:
            MOD_Player_Channel_set_volume(player_channel, player_channel->volume + (x == 0 ? -y : x));
            player_channel->vibrato_tick++;
            break;
        case EFFECT_TREMOLO:
            break;
        case EFFECT_UNUSED:
            break;
        case EFFECT_SET_SAMPLE_OFFSET:
            break;
        case EFFECT_VOLUME_SLIDE:
            MOD_Player_Channel_set_volume(player_channel, player_channel->volume + x == 0 ? -y : x);
            break;
        case EFFECT_POSITION_JUMP:
            break;
        case EFFECT_SET_VOLUME:
            MOD_Player_Channel_set_volume(player_channel, (x<<4) | y);
            break;
        case EFFECT_PATTERN_BREAK:
            break;
        case EFFECT_EXTRAS:
            break;

        case EFFECT_SET_SPEED:

            ;int speed = x*16+y;
            if(speed == 0){
                speed = 1;
            }

            if(speed > 32){
                speed = (24 * speed)/6;
            }

            player->ticks_per_division = speed;  
            break;
        default:
            break;
    }

}

void MOD_Player_Channel_tick(MOD_Player_Channel* player_channel, MOD_Player* player, MOD* mod){
    MOD_Channel* channel = mod->patterns[mod->pattern_table[player->song_position]]->divisions[player->active_division]->channels[player_channel->number];

    MOD_Player_Channel_process_effect(player_channel, player, channel->effect);
}

void MOD_Player_Channel_division(MOD_Player_Channel* player_channel, MOD_Player* player, MOD* mod){
    MOD_Channel* channel = mod->patterns[mod->pattern_table[player->song_position]]->divisions[player->active_division]->channels[player_channel->number];

    if(channel->sample != 0){
        player_channel->sample = mod->samples[channel->sample-1];
        player_channel->sample_period = channel->sample_period;
        player_channel->sample_tracker = 0;
        MOD_Player_Channel_set_volume(player_channel, 64);
    }
}
