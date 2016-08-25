echo Don\'t Run this in the background
echo because it will ask for a password
cp systemd_test_goal_states.txt svcagt_goal_states.txt
rm -f $HOME/mock_systemctl_cmds.txt
rm -f $HOME/mock_systemctl_responses.txt
mkfifo $HOME/mock_systemctl_cmds.txt 
mkfifo $HOME/mock_systemctl_responses.txt
sudo python mock_systemctl.py $HOME sajwt1 sajwt2 sajwt3

