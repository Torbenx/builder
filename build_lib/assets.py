import os
import utils
import shutil

def copy_assets(source_dir, target_dir, logger, rebuild=False):
    any_copy = False
    if not os.path.exists(source_dir):
    	logger.nothing_to_do()
    	return False
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
    for item in os.listdir(source_dir):
        source_file = utils.join(source_dir, item)
        target_file = utils.join(target_dir, item)
        if os.path.isfile(source_file):
            if (not os.path.exists(target_file)) or os.path.getmtime(target_file) < os.path.getmtime(source_file):
                logger.log("Copying "+source_file)
                any_copy = True
                shutil.copyfile(source_file, target_file)
        else:
            if copy_assets(utils.join(source_dir, item), utils.join(target_dir, item), logger, rebuild=rebuild):
                any_copy = True
        if any_copy:
            logger.success()
        else:
            logger.nothing_to_do()
    return any_copy