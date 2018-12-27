import os
import build
import shutil

def copy_assets(source_dir, target_dir):
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
    for item in os.listdir(source_dir):
        source_file = build.join(source_dir, item)
        target_file = build.join(target_dir, item)
        if os.path.isfile(source_file):
            if (not os.path.exists(target_file)) or os.path.getmtime(target_file) < os.path.getmtime(source_file):
                print "Copying "+source_file
                shutil.copyfile(source_file, target_file)
        else:
            copy_assets(build.join(source_dir, item), build.join(target_dir, item))