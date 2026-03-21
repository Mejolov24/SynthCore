import soundfont_generator_library as sfg
import sys
dropped_files = sys.argv[1:]

if not dropped_files:
    print("No files were dropped onto the script.")
    input("Press Enter to exit...") 
else:
    sfg.convert_files(dropped_files,int(input("Sample Rate : ")) , int(input("Bit resolution : ")) )
    print("\n")
        
    
    input("Press Enter to exit...")