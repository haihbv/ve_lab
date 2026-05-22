# Create a folder
$ mkdir actions-runner && cd actions-runner# Download the latest runner package
$ curl -o actions-runner-linux-arm64-2.333.1.tar.gz -L https://github.com/actions/runner/releases/download/v2.333.1/actions-runner-linux-arm64-2.333.1.tar.gz# Optional: Validate the hash
$ echo "69ac7e5692f877189e7dddf4a1bb16cbbd6425568cd69a0359895fac48b9ad3b  actions-runner-linux-arm64-2.333.1.tar.gz" | shasum -a 256 -c# Extract the installer
$ tar xzf ./actions-runner-linux-arm64-2.333.1.tar.gz
bash: $: command not found
bash: $: command not found
bash: $: command not found
Unknown option: #
Type shasum -h for help
bash: $: command not found
pi@lab:~ $ mkdir actions-runner && cd actions-runner
pi@lab:~/actions-runner $ curl -o actions-runner-linux-arm64-2.333.1.tar.gz -L https://github.com/actions/runner/releases/download/v2.333.1/actions-runner-linux-arm64-2.333.1.tar.gz
  % Total    % Received % Xferd  Average Speed   Time    Time     Time  Current
                                 Dload  Upload   Total   Spent    Left  Speed
  0     0    0     0    0     0      0      0 --:--:-- --:--:-- --:--:--     0
100  130M  100  130M    0     0  2272k      0  0:00:58  0:00:58 --:--:-- 2448k
pi@lab:~/actions-runner $ echo "69ac7e5692f877189e7dddf4a1bb16cbbd6425568cd69a0359895fac48b9ad3b  actions-runner-linux-arm64-2.333.1.tar.gz" | shasum -a 256 -c
actions-runner-linux-arm64-2.333.1.tar.gz: OK
pi@lab:~/actions-runner $ tar xzf ./actions-runner-linux-arm64-2.333.1.tar.gz
pi@lab:~/actions-runner $ ./config.sh --url https://github.com/nvu1608/my-autograder --token BXCVGI3NPBYP6HZPDFGCEETJ5AASC

--------------------------------------------------------------------------------
|        ____ _ _   _   _       _          _        _   _                      |
|       / ___(_) |_| | | |_   _| |__      / \   ___| |_(_) ___  _ __  ___      |
|      | |  _| | __| |_| | | | | '_ \    / _ \ / __| __| |/ _ \| '_ \/ __|     |
|      | |_| | | |_|  _  | |_| | |_) |  / ___ \ (__| |_| | (_) | | | \__ \     |
|       \____|_|\__|_| |_|\__,_|_.__/  /_/   \_\___|\__|_|\___/|_| |_|___/     |
|                                                                              |
|                       Self-hosted runner registration                        |
|                                                                              |
--------------------------------------------------------------------------------

# Authentication


√ Connected to GitHub

# Runner Registration

Enter the name of the runner group to add this runner to: [press Enter for Default] 

Enter the name of runner: [press Enter for lab] 

This runner will have the following labels: 'self-hosted', 'Linux', 'ARM64' 
Enter any additional labels (ex. label-1,label-2): [press Enter to skip] 

√ Runner successfully added

# Runner settings

Enter name of work folder: [press Enter for _work] 

√ Settings Saved.

pi@lab:~/actions-runner $ ./run.sh

√ Connected to GitHub

Current runner version: '2.333.1'
2026-04-21 22:02:40Z: Listening for Jobs
^CExiting...
Runner listener exit with 0 return code, stop the service, no retry needed.
Exiting runner...
pi@lab:~/actions-runner $ 


----------------------------------------------------------------------------------------------------------------------------

# 22_4_2026

pi@lab:~/actions-runner $ cd ~/actions-runner
sudo ./svc.sh install
sudo ./svc.sh start
sudo ./svc.sh status
Creating launch runner in /etc/systemd/system/actions.runner.nvu1608-my-autograder.lab.service
Run as user: pi
Run as uid: 1000
gid: 1000
Created symlink '/etc/systemd/system/multi-user.target.wants/actions.runner.nvu1608-my-autograder.lab.service' → '/etc/systemd/system/actions.runner.nvu1608-my-autograder.lab.service'.

/etc/systemd/system/actions.runner.nvu1608-my-autograder.lab.service
● actions.runner.nvu1608-my-autograder.lab.service - GitHub Actions Runner (nvu1608-my-autograder.lab)
     Loaded: loaded (/etc/systemd/system/actions.runner.nvu1608-my-autograder.lab.service; enabled; preset: enabled)
     Active: active (running) since Wed 2026-04-22 14:52:43 +07; 163ms ago
 Invocation: 5772ac0648274ac995bdb7d8caac353c
   Main PID: 4972 (runsvc.sh)
      Tasks: 2 (limit: 756)
        CPU: 65ms
     CGroup: /system.slice/actions.runner.nvu1608-my-autograder.lab.service
             ├─4972 /bin/bash /home/pi/actions-runner/runsvc.sh
             └─4978 ./externals/node20/bin/node ./bin/RunnerService.js

Apr 22 14:52:43 lab systemd[1]: Started actions.runner.nvu1608-my-autograder.lab.service - GitHub Actions Runner (nvu1608-my-autograder.lab).
Apr 22 14:52:43 lab runsvc.sh[4972]: .path=/home/pi/.vscode-server/cli/servers/Stable-41dd792b5e652393e7787322889ed5fdc58bd75b/server/bin/remote-cli:/usr/local/sbin:/usr/local/…e/pi/.local/bin
Hint: Some lines were ellipsized, use -l to show in full.

/etc/systemd/system/actions.runner.nvu1608-my-autograder.lab.service
● actions.runner.nvu1608-my-autograder.lab.service - GitHub Actions Runner (nvu1608-my-autograder.lab)
     Loaded: loaded (/etc/systemd/system/actions.runner.nvu1608-my-autograder.lab.service; enabled; preset: enabled)
     Active: active (running) since Wed 2026-04-22 14:52:43 +07; 324ms ago
 Invocation: 5772ac0648274ac995bdb7d8caac353c
   Main PID: 4972 (runsvc.sh)
      Tasks: 2 (limit: 756)
        CPU: 79ms
     CGroup: /system.slice/actions.runner.nvu1608-my-autograder.lab.service
             ├─4972 /bin/bash /home/pi/actions-runner/runsvc.sh
             └─4978 ./externals/node20/bin/node ./bin/RunnerService.js

Apr 22 14:52:43 lab systemd[1]: Started actions.runner.nvu1608-my-autograder.lab.service - GitHub Actions Runner (nvu1608-my-autograder.lab).
Apr 22 14:52:43 lab runsvc.sh[4972]: .path=/home/pi/.vscode-server/cli/servers/Stable-41dd792b5e652393e7787322889ed5fdc58bd75b/server/bin/remote-cli:/usr/local/sbin:/usr/local/…e/pi/.local/bin
Hint: Some lines were ellipsized, use -l to show in full.
pi@lab:~/actions-runner $ 
