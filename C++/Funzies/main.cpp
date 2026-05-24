#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <ostream>
#include <string>
#include "Bits.h"

using namespace std;

string setBit(int ptrN, string& ptrBitInt) {
  reverse(ptrBitInt.begin(), ptrBitInt.end());
  for (int i = 0; i < ptrBitInt.length(); i++) {
    if (i == ptrN){
     ptrBitInt[i] = '1';
    }
  }
  
  reverse(ptrBitInt.begin(), ptrBitInt.end());
  cout << ptrBitInt << endl;
  return ptrBitInt;
}

string clearBit(int ptrN, string& ptrBitInt) {
  for (int i = 0; i < ptrBitInt.length(); i++) {
     if (i == ptrN){
     ptrBitInt[i] = '0';
    }   
  }
  string ptrBitIntcopy = ptrBitInt;
  reverse(begin(ptrBitIntcopy),end(ptrBitIntcopy));
  cout << ptrBitIntcopy << endl;
  return ptrBitInt;
}

string toggleBit(int ptrN, string& ptrBitInt) {
  for (int i = 0; i < ptrBitInt.length(); i++){
    if (i == ptrN){
      ptrBitInt[i] = ptrBitInt[i] ^ 1;
    }
  }
  string ptrBitIntcopy = ptrBitInt;
  reverse(begin(ptrBitIntcopy), end(ptrBitIntcopy));
  cout << ptrBitIntcopy << endl;
  return ptrBitInt;
}

string checkBit(int ptrN, string& ptrBitInt) {
  string ptrBitIntcopy = ptrBitInt;
  reverse(ptrBitIntcopy.begin(), ptrBitIntcopy.end());
  for (int i = 0; i < ptrBitIntcopy.length(); i++){
    if (i  == ptrN) {
      cout << ptrBitIntcopy[i] << endl;
    }
  }
  
  return ptrBitInt;
}

string popCounter(string& ptrBitInt){
  int oneCounter = 0;
  for (int i = 0; i < ptrBitInt.length(); i++) {
    if (ptrBitInt[i] == '1'){
      oneCounter++;
    }
  }
  cout << oneCounter << endl;
  return ptrBitInt;
}

string leftRotation(string& ptrBitInt){
  char firstBit = ptrBitInt[0];
  ptrBitInt.erase(0, 1);
  ptrBitInt.push_back(firstBit);
  cout << ptrBitInt << endl;
  return ptrBitInt;
}

string rightRotation(string ptrBitInt) {
  char lastBit = ptrBitInt.back();
  ptrBitInt.pop_back();
  ptrBitInt.insert(0, 1, lastBit);
  cout << ptrBitInt << endl;
  return ptrBitInt;
}

int main() {
  string ptrbitInt = "00000000";
  while (true) {
    string operation;
    int n;

    cout << "what operation do you want to perform? setbit, clearbit, togglebit, checkbit, popcounter, leftrotation, rightrotation, arrow up for last operation , bitstring to use a custom binary number or 'quit' to quit. " << "\n";
    string lastOperation = operation;
    cin >> operation;
    if (operation != "setbit" && operation != "clearbit" && operation != "togglebit" && operation != "checkbit" && operation != "popcounter" && operation != "bitstring" && operation != "leftrotation" && operation != "rightrotation" && operation != "^[[A" && operation != "quit"){
      cout<<"unknown operation enter a different operation" << "\n";
      continue;
    }
    
    //without a specific bit
    if (operation == "quit"){
      break;
    }
    if (operation == "^[[A") {
      operation = lastOperation;
    }

    if (operation == "popcounter"){
      popCounter(ptrbitInt);
      continue;
    }
    if (operation == "bitstring"){
      cin >> ptrbitInt;
      reverse(ptrbitInt.begin(), ptrbitInt.end());
      continue;
    }
    if (operation == "leftrotation"){
      leftRotation(ptrbitInt);
      continue;
    }
    if (operation == "rightrotation"){
      rightRotation(ptrbitInt);
      continue;
    }

    //with a specific bit
    cin >> n;
    cin.ignore();
    cin.clear();
    n = n - 1;
    if (n <= 16) { 
      if (operation == "setbit"){
        setBit(n, ptrbitInt);
      }
      if (operation == "clearbit"){
        clearBit(n, ptrbitInt);
      }
      if (operation == "togglebit"){
        toggleBit(n, ptrbitInt);
      }
      if (operation == "checkbit"){
        checkBit(n, ptrbitInt);
      }
    }
  }
  return 0;
}
