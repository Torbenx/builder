import os
import utils
import shutil

def copy_assets(source_dir, target_dir, rebuild=False):
    any_copy = False
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
    for item in os.listdir(source_dir):
        source_file = utils.join(source_dir, item)
        target_file = utils.join(target_dir, item)
        if os.path.isfile(source_file):
            if (not os.path.exists(target_file)) or os.path.getmtime(target_file) < os.path.getmtime(source_file):
                print "Copying "+source_file
                any_copy = True
                shutil.copyfile(source_file, target_file)
        else:
            if copy_assets(utils.join(source_dir, item), utils.join(target_dir, item), rebuild=rebuild):
                any_copy = True
    return any_copy