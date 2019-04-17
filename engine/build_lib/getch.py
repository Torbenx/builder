import os

try:
    import msvcrt
    
    def getch():
        return msvcrt.getch()
        
    def kbhit():
        return msvcrt.kbhit()
        
    def clear():
        os.system("cls")
        
except ImportError:
    
    def getch():
        return raw_input()
        
    def kbhit():
        return False
        
    def clear():
        os.system("clear")
