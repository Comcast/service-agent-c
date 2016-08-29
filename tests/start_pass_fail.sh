rm -f svcagt_goal_states.txt
rm -f $HOME/mock_systemctl_cmds.txt
rm -f $HOME/mock_systemctl_responses.txt
rm -f $HOME/mock_systemctl_msgs.txt
mkfifo $HOME/mock_systemctl_cmds.txt 
mkfifo $HOME/mock_systemctl_responses.txt
python mock_systemctl.py $HOME
