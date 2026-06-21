using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace ImportConsole {
	class Program {
		static void Main(string[] args) {
			TestCalculation();
			Console.ReadLine();
		}

		public static void TestCalculation() {
			var ID = RegisterRouter();
			var Source = File.ReadAllText("Input.txt");
			Console.WriteLine(Execute(CalculateLayout, ID, Source));
			Console.WriteLine(Execute(CalculateRoute, ID, Source));
			UnregisterRouter(ID);
		}

		#region ( General support )
		/// <summary>
		/// ImportFrom: the import source.
		/// </summary>
		private const string ImportFrom = "qcrouter";
		/// <summary>
		/// HelloWorld: native function smoke test.
		/// </summary>
		[DllImport(ImportFrom)]
		public static extern int HelloWorld(byte[] Input, byte[] Output);
		/// <summary>
		/// NativeCache: buffer for native method output.
		/// </summary>
		private static byte[] NativeCache = new byte[4095];
		/// <summary>
		/// Execute: invokes a native method.
		/// </summary>
		public static string Execute(Func<byte[], byte[], int> Method, string Input) {
			var Length = Method.Invoke(Encoding.UTF8.GetBytes(Input), NativeCache);
			return Encoding.UTF8.GetString(NativeCache, 0, Length);
		}
		/// <summary>
		/// Execute: invokes a native method.
		/// </summary>
		public static string Execute<T>(Func<T, byte[], byte[], int> Method, T Data, string Input) {
			var Length = Method.Invoke(Data, Encoding.UTF8.GetBytes(Input), NativeCache);
			return Encoding.UTF8.GetString(NativeCache, 0, Length);
		}
		#endregion

		#region ( Router management )
		/// <summary>
		/// RegisterRouter: registers a circuit-diagram router.
		/// </summary>
		[DllImport(ImportFrom)]
		public extern static int RegisterRouter();
		/// <summary>
		/// UnregisterRouter: unregisters a circuit-diagram router.
		/// </summary>
		[DllImport(ImportFrom)]
		public extern static void UnregisterRouter(int ID);
		/// <summary>
		/// CalculateLayout: computes the layout changes of the given circuit.
		/// </summary>
		[DllImport(ImportFrom)]
		public static extern int CalculateLayout(int ID, byte[] Input, byte[] Output);
		/// <summary>
		/// CalculateRoute: computes the wire routing of the given circuit.
		/// </summary>
		[DllImport(ImportFrom)]
		public static extern int CalculateRoute(int ID, byte[] Input, byte[] Output);
		#endregion
	}
}
