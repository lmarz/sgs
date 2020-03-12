#!/bin/sh
if [ ! -d repos ]
then
  echo "---Create Directory \"repos\"---"
  mkdir repos
  echo "---Create git repository \"test.git\"---"
  git init --bare repos/test.git
  echo "---Add a commit to the repository---"
  git clone repos/test.git
  cd test
  echo "This is some text" > file.txt
  git add -A
  git commit -m "Initial Commit"
  git push
  echo "---Clean up---"
  cd ..
  rm -rf test
fi