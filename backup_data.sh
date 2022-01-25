#!/bin/bash
scp -r fits/* sunip@qe.locsst.uml.edu:share/comic_data_new/
if [ $? -eq 0 ]
then
    rm -rf fits/*
    echo "success"
else
    echo "failure"
fi