#include "CHIP8.h"
#include <fstream>
#include <Windows.h>
#include <cstdio>

const int ROWS = 32;
const int COLS = 64;

const int FONTS [] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80 // F
};

void CHIP8::getInputs() {

    keyStates[0] = GetAsyncKeyState('A') & 0x8000;
    keyStates[1] = GetAsyncKeyState('W') & 0x8000;
    keyStates[2] = GetAsyncKeyState('C') & 0x8000;
    keyStates[3] = GetAsyncKeyState('D') & 0x8000;;

    keyStates[4] = GetAsyncKeyState('S') & 0x8000;
    keyStates[5] = GetAsyncKeyState('F') & 0x8000;
    keyStates[6] = GetAsyncKeyState('G') & 0x8000;
    keyStates[7] = GetAsyncKeyState('H') & 0x8000;

    keyStates[8] = GetAsyncKeyState('I') & 0x8000;
    keyStates[9] = GetAsyncKeyState('J') & 0x8000;
    keyStates[10] = GetAsyncKeyState('K') & 0x8000;
    keyStates[11] = GetAsyncKeyState('L') & 0x8000;

    keyStates[12] = GetAsyncKeyState('M') & 0x8000;
    keyStates[13] = GetAsyncKeyState('N') & 0x8000;
    keyStates[14] = GetAsyncKeyState('O') & 0x8000;
    keyStates[15] = GetAsyncKeyState('P') & 0x8000;
}

bool CHIP8::loadRom(char path[]) {
    std :: ifstream romFile;
    romFile.open(path, std::ios::binary);
    char data;
    int i = 0x200;
    if (romFile.is_open()) {
        while(romFile.read(&data, 1) && i < 4098) {
            memory[i++] = (unsigned char)data;
        }
    }  else {
        return false;
    }
}

CHIP8 :: CHIP8() {
    //std :: cout << "CONSTRUCTED!";
    displayBuffer = new wchar_t[ROWS * COLS + 1]();
    memory = new int[4098]();
    registerSet = new int[16]();
    keyStates = new int[16]();
    pc = 0x200;
    sp = -1;
    stack = new int[16]();
    soundTimer = 0;
    delayTimer = 0;
    index = 0;
    drawFlag = true;
    memcpy(memory, FONTS, sizeof(FONTS));
    consoleHandle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(consoleHandle);
}

void CHIP8::flushBuffer() {
    displayBuffer[ROWS * COLS] = '\0';
    WriteConsoleOutputCharacterW(consoleHandle, displayBuffer, ROWS * COLS, {0 , 0}, &charsWritten);
    drawFlag = false;
}

void CHIP8::cycle() {

    instruction =  (memory[pc] << 8) | memory[pc + 1];
    int opcode = instruction & 0xF000;
    pc += 2;
    int vx = (instruction & 0x0F00) >> 8;
    int vy = (instruction & 0x00F0) >> 4;

    switch(opcode) {
        case 0x0000:
            opcode = instruction & 0x00FF;
            switch(opcode) {

                case 0x00E0:
                    // CLEAR DISPLAY BUFFER
                    delete(displayBuffer);
                    displayBuffer = new wchar_t[ROWS * COLS + 1]();
                    break;
                case 0x00EE:
                    // RETURN FROM A SUBROUTINE
                    pc = stack[sp--];
                    break;
            }
            break;
        case 0x1000:
            // JUMP TO ADDRESS
            pc = instruction & 0x0FFF;
            break;  
        case 0x2000:
            // CALL SUBROUTINE
            stack[++sp] = pc;
            pc = instruction & 0x0FFF;
            break;
        case 0x3000:
            // SKIP NEXT INSTRUCTION IF Vx = KK
            if(registerSet[vx] == (instruction & 0x00FF)) pc += 2;
            break;
        case 0x4000:
            // SKIP NEXT INSTRUCTION IF Vx != KK
            if(registerSet[vx] != (instruction & 0x00FF)) pc += 2;
            break;
        case 0x5000:
            // SKIP NEXT INSTRUCTION IF Vx = Vy
            if(registerSet[vx] == registerSet[vy]) pc += 2;
            break;
        case 0x6000:
            // SET Vx = KK
            registerSet[vx] = instruction & 0x00FF;
            break;
        case 0x7000:
            // SET Vx = Vx + KK
            registerSet[vx] = (registerSet[vx] + (instruction & 0x00FF)) & 0x00FF;
            break;
        case 0x8000:
            opcode = instruction & 0xF00F;
            switch(opcode) {
                case 0x8000:
                    // SET Vx = Vy
                    registerSet[vx] = registerSet[vy];    
                    registerSet[vx] &= 0xFF;
                    break;
                case 0x8001:
                    // SET Vx = Vx OR Vy
                    registerSet[vx] = registerSet[vx] | registerSet[vy]; 
                    registerSet[vx] &= 0xFF;
                    break;
                case 0x8002:
                    // SET Vx = Vx AND Vy
                    registerSet[vx] = registerSet[vx] & registerSet[vy]; 
                    registerSet[vx] &= 0xFF;
                    break;
                case 0x8003:
                    // SET Vx = Vx XOR Vy
                    registerSet[vx] = registerSet[vx] ^ registerSet[vy];
                    registerSet[vx] &= 0xFF;
                    break;
                case 0x8004:
                    // SET Vx = Vx + Vy, Vf = Carry
                    registerSet[vx] += registerSet[vy];
                    registerSet[0xF] = registerSet[vx] >> 8;
                    registerSet[vx] &= 0xFF;
                    break;
                case 0x8005:
                    // SET Vx = Vx - Vy, Vf = Borrow
                    // IF Vx > Vy, SET Vf = 1
                    registerSet[0xF] = (registerSet[vx] > registerSet[vy]) ? 1 : 0;
                    registerSet[vx]  = (registerSet[vx] - registerSet[vy]) & 0x00FF;
                    break;
                case 0x8006:
                    // SET Vx = SHR Vx 1
                    registerSet[0xF] = registerSet[vx] & 0x0001;
                    registerSet[vx]  >>= 1; 
                    break;
                case 0x8007:
                    // SET Vx = Vy - Vx
                    // IF Vy > Vx, SET Vf = 1
                    registerSet[0xF] = registerSet[vy] > registerSet[vx] ? 1 : 0;
                    registerSet[vx]  = (registerSet[vy] - registerSet[vx]) & 0x00FF;
                    break;
                case 0x800E:
                    // SET Vx = SHL Vx 1
                    registerSet[0xF] = (registerSet[vx] & 0x0080) >> 7;
                    registerSet[vx]  <<= 1; 
                    registerSet[vx] &= 0xFF;
                    break;
                default:
                    // Handle Later
                    break;                  
            }
            break;
        case 0x9000:
            // SKIP NEXT INSTRUCTION IF Vx != Vy
            if(registerSet[vx] != registerSet[vy]) pc += 2;
            break;
        case 0xA000:
            // SET INDEX = nnn
            index = instruction & 0x0FFF;
            break;
        case 0xB000:
            // JUMP TO nnn + V0
            pc = registerSet[0x0] + (instruction & 0x0FFF);
            break;
        case 0xC000:
            // SET Vx = RANDOM(255) & KK
            registerSet[vx] = (rand() % 2565) & (instruction & 0x00FF);
            registerSet[vx] &= 0xFF;
            break;
        case 0xD000:
            // Draw Sprite
            registerSet[0xF] = 0;
            x = registerSet[vx] & 0xFF;
            y = registerSet[vy] & 0xFF;
            for(int i = 0, max = instruction & 0x000F; i < max ; i++) {
                byte = memory[i + index];
                coordY = i + y;                
                for(int j = 0 ; j < 8 ; j++) {
                    coordX = j + x;
                    bit = ((byte >> (7 - j)) & 0x0001);
                    if(bit == 0x0 || coordY > ROWS  || coordX > COLS  || coordX < 0|| coordY < 0 ) {
                        continue;
                    }
                    coordIndex = coordY * COLS + coordX;
                    if(displayBuffer[coordIndex] == 0x2588) {
                        displayBuffer[coordIndex] = 0;
                        registerSet[0xF] = 1;
                    } else {
                        displayBuffer[coordIndex] = 0x2588;
                    }
                }
            }
            drawFlag = true;
            break;
        case 0xE000:
            opcode = instruction & 0xF0FF;
            //printf("%x ", opcode);
            switch(opcode) {
                case 0xE09E:
                    // Skip Next Instruction If Vx Is Pressed
                    if(keyStates[registerSet[vx]] == 0x8000) pc += 2;
                    break;
                case 0xE0A1:
                    // Skip Next Instruction If Vx Is Not Pressed
                    if(keyStates[registerSet[vx]] == 0x0000) pc += 2;
                    break;
                default:
                    // HANDLE
                    break;
            }
        case 0xF000:
            opcode = instruction & 0xF0FF;
            //printf("%x ", opcode);
            switch(opcode) {
                case 0xF007:
                    // SET Vx = Delay Timer
                    registerSet[vx] = delayTimer;
                    break;
                case 0xF00A:
                    // Wait for keypress, Store the key in Vx
                    offset = 2;
                    for(int i = 0 ; i < 16 ; i++) {
                         if(keyStates[i] == 0x8000) {
                             registerSet[vx] = i;
                             offset = 0;
                             break;
                         }
                    }
                    pc -= offset;
                    break;
                case 0xF015:
                    // SET Delay Timer = Vx
                    delayTimer = registerSet[vx];
                    break;
                case 0xF018:
                    // SET Sound Timer = Vx
                    soundTimer = registerSet[vx];
                    break;
                case 0xF01E:
                    // SET I = I + Vx
                    index =  (index + registerSet[vx]);
                    if(index > 0x0FFF) registerSet[0xF] = 1;
                    else registerSet[0xF] = 0;
                    index &= 0x0FFF;
                    break;
                case 0xF029:
                    // SET I = Location of digit in Vx
                    index = (registerSet[vx] * 5) & 0xFFF;
                    break;
                case 0xF033:
                    // Store BCD Representation of Vx in Memory at I, I+1, I+2
                    digit = registerSet[vx];
                    memory[index] =     digit / 100;
                    memory[index + 1] = (digit / 10) % 10;
                    memory[index + 2] = digit % 10; 
                    break;

                case 0xF055:
                    // FLUSH REGISTERS V0 To Vx in memory at Index I
                    memcpy(memory + index, registerSet, (vx + 1) * sizeof(int));
                    break;
                case 0xF065:
                    // LOAD REGISTERS V0 To Vx From memory at Index I
                    memcpy(registerSet, memory + index, (vx + 1) * sizeof(int));
                    break;
                default:
                    // HANDLE
                    break;                    
            }
            
    }
    
    if(delayTimer > 0) delayTimer--;
    if(soundTimer > 0) {
        // PLAY BUZZER
        std :: cout << '\a';
        soundTimer--;
    }
}