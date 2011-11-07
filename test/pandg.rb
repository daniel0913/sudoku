i=0
while gets and i+=1
  open("grid#{i}", "w") do |f|
    $_.gsub('0','_').chars.each_slice(9) {|s| f.write(s.join + "\n")}
  end
end
