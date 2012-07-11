import java.io.*;
import java.lang.*;
import stat.*;

public class Sample {
	public static void main( String[] args ) {
		System.out.println( "start" );
		AsqlStat a = new AsqlStat();
	
	 	String asql =  "pred\nlong x[10,10]\nfrom odbc:court_cx,spgl,spgl123,select * from AJJBXX\n" +
			 "cond\nbegin\n" +
			 "stat x\n" +
			 "#y1: 1\n" +
			 "#x1:  1\n" +
			 "end\n" +
			 "act warray( \"c:\\\\hello\", x)";
			 
		System.out.println( a.getResult( "localhost", "admin", "admin", asql ) );
		System.out.println( "end" );
	}
}