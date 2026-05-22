python3 /home/pi/ve_lab/logic/scripts/main.py --lab <lab_directory_name>

python3 /home/pi/ve_lab/logic/scripts/main.py --lab lab6_ds1307

python3 /home/pi/ve_lab/logic/scripts/main.py --lab lab6_ds1307 --build

python3 /home/pi/ve_lab/logic/scripts/main.py --lab lab6_ds1307 --flash

python3 /home/pi/ve_lab/logic/scripts/main.py --lab lab6_ds1307 --reg

python3 /home/pi/ve_lab/logic/scripts/main.py --lab lab6_ds1307 --capture --time 20

python3 /home/pi/ve_lab/logic/scripts/main.py --lab lab6_ds1307 --grade


GITHUB_WORKSPACE=<path_to_parent_directory> python3 /home/pi/ve_lab/logic/scripts/main.py --lab <lab_name> --build

GITHUB_WORKSPACE=/home/pi/actions-runner/_work/my_autograder/my_autograder python3 /home/pi/ve_lab/logic/scripts/main.py --lab lab6_ds1307
