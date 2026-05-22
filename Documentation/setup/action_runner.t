# b1: lay token tren github 
my-autograder → Settings → Actions → Runners → New self-hosted runner

# chon Linux → ARM64

# github genarated 
# Download

# Create a folder
mkdir actions-runner && cd actions-runnerCopied!# Download the latest runner package
curl -o actions-runner-linux-arm64-2.333.1.tar.gz -L https://github.com/actions/runner/releases/download/v2.333.1/actions-runner-linux-arm64-2.333.1.tar.gzCopied!# Optional: Validate the hash
echo "69ac7e5692f877189e7dddf4a1bb16cbbd6425568cd69a0359895fac48b9ad3b  actions-runner-linux-arm64-2.333.1.tar.gz" | shasum -a 256 -cCopied!# Extract the installer
tar xzf ./actions-runner-linux-arm64-2.333.1.tar.gz

# Configure
# Create the runner and start the configuration experience
./config.sh --url https://github.com/nvu1608/my-autograder --token BXCVGI3NPBYP6HZPDFGCEETJ5AASCCopied!# Last step, run it!
./run.sh

# Using your self-hosted runner
# Use this YAML in your workflow file for each job
runs-on: self-hosted


sudo ~/actions-runner/svc.sh status

cd ~/actions-runner
sudo ./svc.sh install
sudo ./svc.sh start
sudo ./svc.sh status