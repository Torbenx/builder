try:
	import msvcrt
	
	def getch():
		return msvcrt.getch()
		
	def kbhit():
		return msvcrt.kbhit()
		
except ImportError:
    
    def getch():
        s = raw_input()
        return s[0]
        
    def kbhit():
        return False
