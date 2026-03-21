import os

# sys.argv is a list of command-line arguments
# sys.argv[0] is the path to the script itself
# sys.argv[1:] contains the paths of the dropped files

run_path = os.path.dirname(os.path.abspath(__file__))

import ffmpeg

def convert_to_pcm(input_file, sample_rate, bit_depth):
    output_file = os.path.join(
    run_path,
    "cache",
    os.path.basename(input_file) + ".raw"
)

    codec = 'pcm_s16le' if bit_depth == 16 else 'pcm_u8'
    fmt = 's16le' if bit_depth == 16 else 'u8'

    (
        ffmpeg
        .input(input_file)
        .output(
            output_file,
            format=fmt,
            acodec=codec,
            ac=1,              # mono
            ar=sample_rate     # sample rate
        )
        .overwrite_output()
        .run()
    )

    return output_file


def check_cache(sample_rate,bit_depth):
    #get latest cache text to see if settings match
    cache_dir = os.path.join(run_path, "cache")
    cache_file = os.path.join(cache_dir, "cache.txt")

    cache = get_cache()
    
    if cache == [sample_rate,bit_depth]:
        return False # No new settings
    else:
        with open(cache_file, "w", encoding="utf-8") as f:
            f.write(f"{sample_rate};{bit_depth};")
        return True # new settings

def get_cache():
    cache_dir = os.path.join(run_path, "cache")
    cache_file = os.path.join(cache_dir, "cache.txt")
    with open(cache_file, "r", encoding="utf-8") as f:
        content = f.read()  
        cache = content.strip().split(";")[:2]
        cache = [int(cache[0]), int(cache[1])]
    return cache
    


def convert_files(files,sample_rate,bit_depth):
    new_settings : bool = check_cache(sample_rate,bit_depth)

    if new_settings:
        print("Settings changed, overwritting existing files...")
    else:
         print("No settings changed, skiping existing files...")

    print("\n")

    for file_path in files:

        file_name = os.path.basename(file_path)
        cache_file_path = os.path.join(run_path, "cache",f"{file_name}.raw")

        if new_settings:
            convert_to_pcm(file_path,sample_rate,bit_depth)
           
        elif not os.path.exists(cache_file_path):
            
            convert_to_pcm(file_path,sample_rate,bit_depth)
    
    print("\n")
    print("Converted succesfully!")



 



