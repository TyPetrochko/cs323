test: Token
	/c/cs323/bin/testit 3 Token

mytests: Token
	cat Examples/MyTests/t14 | ./Token > Examples/MyTests/t14.me
	cat Examples/MyTests/t14 | /c/cs323/Hwk3/Token > Examples/MyTests/t14.stan
	cmp Examples/MyTests/t14.me Examples/MyTests/t14.stan
	cat Examples/MyTests/t19 | ./Token > Examples/MyTests/t19.me
	cat Examples/MyTests/t19 | /c/cs323/Hwk3/Token > Examples/MyTests/t19.stan
	cmp Examples/MyTests/t19.*
	cat Examples/MyTests/t20 | ./Token > Examples/MyTests/t20.me
	cat Examples/MyTests/t20 | /c/cs323/Hwk3/Token > Examples/MyTests/t20.stan
	cmp Examples/MyTests/t20.*
