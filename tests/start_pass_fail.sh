echo Don\'t Run this in the background 
echo because it will ask for a password
rm -f svcagt_goal_states.txt
rm -f $HOME/mock_systemctl_cmds.txt
rm -f $HOME/mock_systemctl_responses.txt
mkfifo $HOME/mock_systemctl_cmds.txt 
mkfifo $HOME/mock_systemctl_responses.txt
sudo python mock_systemctl.py $HOME
