for i in {1..2000}
do
  echo "Test(" $i ") ................................................. :-" >> final_result.out
  ./caltrain >> final_result.out
done