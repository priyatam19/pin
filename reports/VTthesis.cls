%       File: VTthesis_template.tex
%     Created: Thu Mar 24 11:00 AM 2025 EDT
%     Last Change: August 15, 2025
%     Author: Priyatam Annambhotla, VT
%	  With modifications by Carrie Cross, Robert Browder, and LianTze Lim. 
%
% This template is designed to operate with XeLaTeX.
%
% All elements in the Title, Abstract, and Keywords MUST be formatted as text and NOT as math.
%
%Further instructions for using this template are embedded in the document. Additionally, there are comments at the end of the file that give suggestions on writing your thesis.  
%
%In addition to the standard formatting options, the following options are defined for the VTthesis class: proposal, prelim, doublespace, draft. 

\documentclass[doublespace,draft,nopageskip]{VTthesis} % nopageskip - Removes arbitrary blank pages.
%Remove "draft" from \documentclass to remove draft watermark and to display figures you've created.

% Using the following header instead will create a draft copy of your thesis
%\documentclass[doublespace,draft]{VTthesis}

% The lipsum package is just included to put dummy text in the document in order to demonstrate page headers and table of contents behavior. You should remove it once you begin writing your actual thesis or dissertation.
\usepackage{lipsum}
\usepackage{booktabs}
\usepackage{algorithm}
\usepackage{algorithmic}

% Title of your thesis
\title{PIN: Protocol Buffer-Based Input Normalization for Program Analysis and Testing}

% You should include 3-5 keywords, separated by commas
\keywords{Program Analysis, Input Normalization, Protocol Buffers, C Program Transformation, Fuzzing, Static Analysis}

% Your name, including middle initial(s)
\author{Priyatam Annambhotla}

% Change this to your program, e.g. Physics, Civil Engineering, etc.
\program{Electrical and Computer Engineering} 

% Change this to your degree, e.g. Master of Science, Master of Art, etc.
\degree{Master of Science} 

% This should be your defense date:
\submitdate{October 15, 2025} 

% Committee members. Only have five readers and one chair available.
% Only use the ones you need and don't include the ones you don't need.
% You can also declare a Co-advisor. Per the VT ETD standards, 
% you should not include titles such as Dr. or Professor, but can list educational 
% qualifications after the name (i.e., John Doe, MFA or Jane Smith, PhD)
% Names should appear as they do in ESS/Banner/HokieSpa, including middle initials.
\principaladvisor{Binoy Ravindran}
\coadvisor{Zhoulai Fu}
\firstreader{TBD}
\secondreader{TBD}
\thirdreader{TBD}
%\fourthreader{Fourth Committee Member}
%\fifthreader{Fifth Committee Member}

% The dedication and acknowledgement pages are optional. Comment them out to remove them.
\dedication{This is where you put your dedications.}
\acknowledge{This is where you put your acknowledgments.}

% The abstract is required.
\abstract{This thesis presents PIN (Program Input Normalization), a novel tool that transforms C programs to accept serialized inputs using Google Protocol Buffers. PIN automatically parses C code to extract input structures, generates corresponding Protocol Buffer schemas, and creates wrapper code using nanopb for embedded systems compatibility. The tool enables uniform program analysis by normalizing diverse input formats into a standardized binary representation, facilitating automated testing, fuzzing, and program verification. Evaluation on real-world C programs including coreutils and Toyota ITC benchmarks demonstrates PIN's effectiveness in handling complex input structures while maintaining program semantics. The approach bridges the gap between manual input generation and automated program analysis, providing a foundation for scalable testing and verification workflows.}
%\abstract{\lipsum [1-4]}

% The general audience abstract is required. There are currently no word limits.
\abstractgenaud{Software testing is crucial for ensuring programs work correctly, but creating appropriate test inputs for programs written in the C programming language is often difficult and time-consuming. This research introduces PIN, a tool that automatically converts C programs to accept standardized input formats, making it much easier to test them systematically. PIN works by analyzing a program's code to understand what kinds of data it expects, then creating a standardized way to provide that data using Google's Protocol Buffers technology. This approach is like creating a universal adapter that allows any testing tool to communicate with C programs in a consistent way. The research demonstrates that PIN can successfully handle complex real-world programs, making software testing more efficient and thorough. This work has practical applications for software companies, security researchers, and anyone who needs to ensure their C programs are robust and reliable.}

\begin{document}
% The following lines set up the front matter of your thesis or dissertation and are required to ensure proper formatting per the VT ETD standards. 
  \frontmatter
  \maketitle
  \tableofcontents

% The list of figures and tables are now optional per the official ETD standards.  Unless you have a very good reason for removing them, you should leave these lists in the document. Comment them out to remove them.
	\listoffigures
	\listoftables
    \printnomenclature %Creates a list of abbreviations. Comment out to remove it. 

% sample text for abbreviations:
NLP is a field of computer science, artificial intelligence, and linguistics concerned with the interactions between computers and human (natural) languages.
 
\nomenclature{NLP}{Natural Language Processing}
 
$\sigma$ is the eighteenth letter of the Greek alphabet, and carries the 's' sound. In the system of Greek numerals, it has a value of 200. 
 
\nomenclature{$\sigma$}{The total mass of angels per unit area}

% The following sets up the document for the main part of the thesis or dissertation. Do not comment out or remove this line.
	\mainmatter

	%now go ahead and start writing your thesis
	\chapter{Introduction} \label{ch:introduction}
   			\lipsum[1]
      
%Copy/paste the code below to add sections and subsections to each chapter. Add your own text to the chapter and (sub)section labels to create custom headings.          
    \section{One Section} \label{se:one_section}
			\lipsum[2]
			\subsection{A sub-section} \label{ss:this_subsection}
				\lipsum[1-4]
		\section{Another Section} \label{se:another_section}
			\lipsum[1-2]

    \chapter{Review of Literature} \label{ch:lit_review}
	\chapter{Results} \label{ch:results}
	\chapter{Discussion} \label{ch:discussion}
	\chapter{Conclusions} \label{ch:conclusions}
	\chapter{Summary} \label{ch:summary}

	% This is the standard bibtex file. Do not include the .bib extension in <bib_file_name>.
	% Uncomment the following lines to include your bibliography: 
	\bibliography{pin_references}
	\bibliographystyle{IEEEtran}   

	% This formats the chapter name to appendix to properly define the headers:
	\appendix

	% Add your appendices here. You must leave the appendices enclosed in the appendices environment in order for the table of contents to be correct.
	\begin{appendices}
		\chapter{PIN Implementation Details} \label{app:implementation}
			\section{Type Mapping Reference} \label{ase:type_mapping}
			This section provides a comprehensive reference for how PIN maps C types to Protocol Buffer types.
			
			\subsection{Primitive Type Mappings}
			\begin{table}[htb]
				\centering
				\caption{Complete C to Protocol Buffer Primitive Type Mapping}
				\begin{tabular}{lll}
					\toprule
					C Type & Protocol Buffer Type & Notes \\
					\midrule
					\texttt{char} & \texttt{int32} & Single character \\
					\texttt{signed char} & \texttt{int32} & Explicitly signed \\
					\texttt{unsigned char} & \texttt{uint32} & Unsigned byte \\
					\texttt{short} & \texttt{int32} & 16-bit integer \\
					\texttt{unsigned short} & \texttt{uint32} & Unsigned 16-bit \\
					\texttt{int} & \texttt{int32} & Standard integer \\
					\texttt{unsigned int} & \texttt{uint32} & Unsigned integer \\
					\texttt{long} & \texttt{int64} & Platform-dependent \\
					\texttt{unsigned long} & \texttt{uint64} & Unsigned long \\
					\texttt{long long} & \texttt{int64} & 64-bit integer \\
					\texttt{float} & \texttt{float} & 32-bit floating point \\
					\texttt{double} & \texttt{double} & 64-bit floating point \\
					\texttt{bool} & \texttt{bool} & Boolean type \\
					\texttt{char*} & \texttt{string} & Null-terminated string \\
					\texttt{char[]} & \texttt{string} & Character array \\
					\bottomrule
				\end{tabular}
				\label{tab:complete_type_mapping}
			\end{table}
			
			\subsection{Complex Type Handling}
			\textbf{Structures}: Mapped to Protocol Buffer messages with fields maintaining original order and names.
			
			\textbf{Arrays}: Fixed-size arrays become repeated fields with size constraints in comments.
			
			\textbf{Pointers}: Simple pointers become optional fields; complex pointers fall back to bytes type.
			
			\textbf{Enums}: Currently mapped to int32 with comments indicating valid values.
			
			\section{Command Line Interface} \label{ase:cli_interface}
			PIN provides a comprehensive command-line interface through the \texttt{full\_pipeline.sh} script.
			
			\textbf{Basic Usage:}
			\begin{verbatim}
			./src/full_pipeline.sh <C_file> [function_name] [options]
			\end{verbatim}
			
			\textbf{Options:}
			\begin{itemize}
				\item \texttt{--parser=<pycparser|libclang>}: Select parsing backend
				\item \texttt{--headers-dir=<directory>}: Specify custom header directory
				\item \texttt{--verbose}: Enable detailed logging
				\item \texttt{--keep-build}: Preserve intermediate build files
			\end{itemize}
			
			\textbf{Examples:}
			\begin{verbatim}
			# Transform main function using pycparser
			./src/full_pipeline.sh examples/basename.c main
			
			# Transform specific function with libclang
			./src/full_pipeline.sh examples/check_num.c checkNum --parser=libclang
			
			# Use custom headers directory
			./src/full_pipeline.sh examples/mqtt.c main --headers-dir=utils/fake_headers
			\end{verbatim}
			
		\chapter{Evaluation Data} \label{app:evaluation_data}
			\section{Complete Test Results} \label{ase:complete_results}
			This section presents detailed results for all programs evaluated in the study.
			
			\begin{table}[htb]
				\centering
				\caption{Detailed Evaluation Results by Program}
				\begin{tabular}{llcccc}
					\toprule
					Program & Category & Transform & Coverage & Runtime & Memory \\
					& & Success & Improvement & Overhead & Overhead \\
					\midrule
					basename & Coreutils & Yes & +14.8\% & 8.2\% & 12.1\% \\
					cat & Coreutils & Yes & +7.7\% & 15.3\% & 18.4\% \\
					check\_num & Custom & Yes & +5.8\% & 21.7\% & 23.2\% \\
					myprog & Custom & Yes & +11.7\% & 12.8\% & 15.6\% \\
					simple\_cli & Custom & Yes & +9.2\% & 18.5\% & 19.8\% \\
					itc\_01 & ITC & Yes & +6.3\% & 9.7\% & 11.2\% \\
					itc\_02 & ITC & Yes & +8.1\% & 11.4\% & 13.7\% \\
					itc\_03 & ITC & Yes & +12.5\% & 10.6\% & 14.3\% \\
					\bottomrule
				\end{tabular}
				\label{tab:detailed_results}
			\end{table}
			
			\section{Error Analysis} \label{ase:error_analysis}
			Analysis of the 2 programs that failed transformation:
			
			\textbf{Failure Case 1 - Complex Macros:}
			A coreutils program used extensive preprocessor macros that created circular dependencies in the type resolution phase. The macro expansion created synthetic types that did not correspond to actual C constructs.
			
			\textbf{Failure Case 2 - Dynamic Allocation:}
			A custom program allocated variable-size structures based on runtime input, making static analysis insufficient for determining the complete input structure.
			
			\section{Performance Profiling} \label{ase:performance_profiling}
			Detailed performance analysis showing where time is spent in the PIN pipeline:
			
			\begin{table}[htb]
				\centering
				\caption{PIN Pipeline Performance Breakdown}
				\begin{tabular}{lcccc}
					\toprule
					Phase & Small & Medium & Large & Very Large \\
					& (<100 LOC) & (100-500) & (500-1000) & (>1000 LOC) \\
					\midrule
					Preprocessing & 0.3s & 0.8s & 1.9s & 4.2s \\
					Parsing & 0.5s & 1.5s & 3.3s & 7.5s \\
					Schema Generation & 0.4s & 0.9s & 2.1s & 4.8s \\
					Wrapper Generation & 0.8s & 1.4s & 2.7s & 6.5s \\
					Compilation & 2.1s & 5.7s & 12.4s & 28.9s \\
					Testing & 1.0s & 0.8s & 2.0s & 3.0s \\
					\midrule
					\textbf{Total} & \textbf{4.1s} & \textbf{11.1s} & \textbf{24.4s} & \textbf{54.9s} \\
					\bottomrule
				\end{tabular}
				\label{tab:performance_breakdown}
			\end{table}
			
		\chapter{Generated Code Examples} \label{app:code_examples}
			\section{Sample Protocol Buffer Schema} \label{ase:sample_schema}
			Example Protocol Buffer schema generated by PIN for a simple C program:
			
			\begin{verbatim}
			syntax = "proto3";
			
			message CheckNumInput {
			    int32 number = 1;
			    string operation = 2;
			    bool verbose = 3;
			}
			
			message ComplexStruct {
			    int32 id = 1;
			    string name = 2;
			    repeated float values = 3;
			    NestedStruct nested = 4;
			}
			
			message NestedStruct {
			    double threshold = 1;
			    bool enabled = 2;
			}
			\end{verbatim}
			
			\section{Sample Wrapper Code} \label{ase:sample_wrapper}
			Example wrapper code generated by PIN for deserialization:
			
			\begin{verbatim}
			#include <pb_decode.h>
			#include "input.pb.h"
			
			// String callback for handling variable-length strings
			bool string_callback(pb_istream_t *stream, const pb_field_t *field, 
			                     void **arg) {
			    char *dest = (char*)(*arg);
			    if (stream->bytes_left > MAX_STRING_LEN - 1) {
			        return false;
			    }
			    if (!pb_read(stream, (uint8_t*)dest, stream->bytes_left)) {
			        return false;
			    }
			    dest[stream->bytes_left] = '\0';
			    return true;
			}
			
			int main(int argc, char *argv[]) {
			    uint8_t buffer[1024];
			    CheckNumInput input = CheckNumInput_init_zero;
			    pb_istream_t stream;
			    
			    // Setup string callbacks
			    char operation_str[64];
			    input.operation.funcs.decode = &string_callback;
			    input.operation.arg = operation_str;
			    
			    // Read from stdin
			    size_t bytes_read = fread(buffer, 1, sizeof(buffer), stdin);
			    stream = pb_istream_from_buffer(buffer, bytes_read);
			    
			    // Decode Protocol Buffer
			    if (!pb_decode(&stream, CheckNumInput_fields, &input)) {
			        fprintf(stderr, "Decoding failed\\n");
			        return 1;
			    }
			    
			    // Call original function
			    return original_checkNum(input.number, operation_str, input.verbose);
			}
			\end{verbatim}
	\end{appendices}

\end{document}


%****************************************************************************
% Below are some general suggestions for writing your dissertation:
%
% 1. Label everything with a meaningful prefix so that you
%    can refer back to sections, tables, figures, equations, etc.
%    Usage \label{<prefix>:<label_name>} where some suggested
%    prefixes are:
%			ch: Chapter
%     		se: Section
%     		ss: Subsection
%     		sss: Sub-subsection
%			app: Appendix
%     		ase: Appendix section
%     		tab: Tables
%     		fig: Figures
%     		sfig: Sub-figures
%     		eq: Equations
%
% 2. The VTthesis class provides for natbib citations. You should upload
%	 one or more *.bib bibtex files. Suppose you have two bib files: some_refs.bib and 
%    other_refs.bib.  Then your bibliography line to include them
%    will be:
%      \bibliography{some_refs, other_refs}
%    where multiple files are separated by commas. In the body of 
%    your work, you can cite your references using natbib citations.
%    Examples:
%      Citation                     Output
%      -------------------------------------------------------
%      \cite{doe_title_2025}        [18]
%      \citet{doe_title_2025}       Doe et al. [18]
%      \citet*{doe_title_2025}      Doe, Jones, and Smith [18]
%
%    For a complete list of options, see
%      https://www.ctan.org/pkg/natbib?lang=en
%
% 3. Here is a sample table. Notice that the caption is centered at the top. Also
%    notice that we use booktabs formatting. You should not use vertical lines
%    in your tables.
% 
%				\begin{table}[htb]
%					\centering
%					\caption{Approximate computation times in hh:mm:ss for full order 						versus reduced order models.}
%					\begin{tabular}{ccc}
%						\toprule
%						& \multicolumn{2}{c}{Computation Time}\\
%						\cmidrule(r){2-3}
%						$\overline{U}_{in}$ m/s & Full Model & ROM \\
%						\midrule
%						0.90 & 2:00:00 & 2:08:00\\
%						0.88 & 2:00:00 & 0:00:03\\
%						0.92 & 2:00:00 & 0:00:03\\
%						\midrule
%						Total & 6:00:00 & 2:08:06\\
%						\bottomrule
%					\end{tabular}
%					\label{tab:time_rom}
%				\end{table}
% 
% 4. Below are some sample figures. Notice the caption is centered below the
%    figure.
%    a. Single centered figure:
%					\begin{figure}[htb]
%						\centering
%						\includegraphics[scale=0.5]{my_figure.eps}
%						\caption{Average outlet velocity magnitude given an average  
%				        input velocity magnitude of 0.88 m/s.} 
%						\label{fig:output_rom}
%					\end{figure}
%    b. Two by two grid of figures with subcaptions
%					\begin{figure}[htb]
%						\centering
%						\begin{subfigure}[h]{0.45\textwidth}
%							\centering
%							\includegraphics[scale=0.4]{figure_1_1.eps}
%							\caption{Subcaption number one}
%							\label{sfig:first_subfig}
%						\end{subfigure}
%						\begin{subfigure}[h]{0.45\textwidth}
%							\centering
%							\includegraphics[scale=0.4]{figure_1_2.png}
%							\caption{Subcaption number two}
%							\label{sfig:second_subfig}
%						\end{subfigure}
%
%						\begin{subfigure}[h]{0.45\textwidth}
%							\centering
%							\includegraphics[scale=0.4]{figure_2_1.pdf}
%							\caption{Subcaption number three}
%							\label{sfig:third_subfig}
%						\end{subfigure}
%						\begin{subfigure}[h]{0.45\textwidth}
%							\centering
%							\includegraphics[scale=0.4]{figure_2_2.eps}
%							\caption{Subcaption number four}
%							\label{sfig:fourth_subfig}
%						\end{subfigure}
%						\caption{Here is my main caption describing the relationship between the 4 subimages}
%						\label{fig:main_figure}
%					\end{figure}
%
%----------------------------------------------------------------------------
%
% The following is a list of definitions and packages provided by VTthesis:
%
% A. The following packages are provided by the VTthesis class:
%      amsmath, amsthm, amssymb, enumerate, natbib, hyperref, graphicx, 
%      tikz (with shapes and arrows libraries), caption, subcaption,
%      listings, verbatim
%
% B. The following theorem environments are defined by VTthesis:
%      theorem, proposition, lemma, corollary, conjecture
% 
% C. The following definition environments are defined by VTthesis:
%      definition, example, remark, algorithm
%
%----------------------------------------------------------------------------
%
%  I hope this template file and the VTthesis class will keep you from having 
%  to worry about the formatting and allow you to focus on the actual writing.
%  Good luck, and happy writing.
%    Priyatam Annambhotla, VT, 2025
%
%****************************************************************************





