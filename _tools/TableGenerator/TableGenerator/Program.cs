using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace TableGenerator
{
    class Program
    {
        /*
\begin{table}[H]
\centering
\caption{Test CausalVisibility - TOB - N° 3 \label{tab:test52}}% table caption, ref label
\begin{tabular}{@{}llllllllllllll@{}}
\toprule
\textbf{C1} & PULL &  & W1 & PUSH &  &  & W4 & PUSH & R5 &  &  &  & R5 \\ \midrule
\textbf{C2} &  & PULL &  &  & W3 & R3 &  &  &  & R3 & PUSH & R3 &  \\ \bottomrule
\end{tabular}
\end{table}
        */

        static void Main(string[] args)
        {
            int count = 1;
            string line;
            List<string> tables = new List<string>();
            List<string> c1 = new List<string>();
            List<string> c2 = new List<string>();
            string guarantee = "";
            string agregar = "";
            // Read the file and display it line by line.
            string path = @"D:\Users\Nicolas\Documents\UBA\Tesis\code\release\gsp\";
            //string path = @"C:\Users\nicol\Documents\UBA\Tesis\repo\code\release\gsp\";
            string input = path + "output.txt";
            string output = path + "OutputWformat.txt";
            StreamReader file =
                new StreamReader(input);
            string table = "";
            bool encontreFecha = false;

            while ((line = file.ReadLine()) != null)
            {
                if (!encontreFecha && !DateTime.TryParse(line, out DateTime date))
                {
                    continue;
                }

                if (!encontreFecha)
                {
                    encontreFecha = true;
                    continue;
                }


                var title = line.Split(' ');
                if (title.Length > 1)
                {
                    if (!string.IsNullOrWhiteSpace(table))
                    {
                        table += @"
\end{tabular}
\end{center}
\end{center}
\end{table}
";
                        /*table = table.Replace("FORMAT", new String('l', c1.Count + 1))
                            .Replace("CLIENTE1", string.Join(" & ", c1)).Replace("CLIENTE2", string.Join(" & ", c2));
                        tables.Add(table);*/
                    }

                    table += @"
%" + title[0] + @"
\begin{table}[H]
\centering
\caption{Resultado N° " + count + @" \label{tab:result" + count++ + @"}}
\begin{center}
\begin{tabular}{|c|c|}
\hline
\textbf{C1} & \textbf{C2} \\ \hline";
                    /*c1 = new List<string>();
                    c2 = new List<string>();

                    if (guarantee != title[0])
                    {
                        guarantee = title[0];
                        agregar = @"%" + guarantee;
                    }
                    else
                    {
                        agregar = "";
                    }
                    table = agregar + @"
\begin{table}[H]
\centering
\caption{Resultados del experimento N° " + title[1] + @" \label{tab:test" + count++ + @"}}
\begin{tabular}{@{}FORMAT@{}}
\toprule
\textbf{C1} & CLIENTE1 \\ \midrule
\textbf{C2} & CLIENTE2 \\ \bottomrule
\end{tabular}
\end{table}";*/
                    file.ReadLine();
                }
                else
                {
                    string client = line.Substring(1, 1);
                    string op = line.Substring(3);
                    table += @"
" + (client == "1" ? op + @" & " : @" & " + op) + @" \\ \hline";
                    /*if (client == "1")
                    {

                        c1.Add(op);
                        c2.Add("");
                    }
                    else
                    {
                        c1.Add("");
                        c2.Add(op);
                    }*/
                }
            }
            table += @"
\end{tabular}
\end{center}
\end{table}
";
            /*table = table.Replace("FORMAT", new String('l', c1.Count + c2.Count))
                .Replace("CLIENTE1", string.Join(" & ", c1)).Replace("CLIENTE2", string.Join(" & ", c2));
            tables.Add(table);*/

            file.Close();

            using (System.IO.StreamWriter file2 =
                new System.IO.StreamWriter(output))
            {

                file2.WriteLine(table);
            }

            // Suspend the screen.  
            //System.Console.ReadLine();
        }
    }
}
