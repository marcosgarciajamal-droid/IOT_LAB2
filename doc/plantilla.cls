\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{plantilla}[2022/5/14]
\LoadClass[a4paper,
11pt,
DIV=13,
headings=normal,
% parskip=full-,
abstract=true,
twoside=semi
]{scrartcl}


%% Opcions de la classe

% L'opció indica si és un informe d'estudiant o, per omissió,
% un enunciat de pràctiques
\newif\if@informe \@informefalse
\DeclareOption{informe}{\@informetrue}

% llengua
\newif\if@practi@catalan  \@practi@catalantrue
\DeclareOption{catalan}{\@practi@catalantrue}
\DeclareOption{english}{\@practi@catalanfalse}

\ProcessOptions\relax

% Processa la llengua
\if@practi@catalan
  \PassOptionsToPackage{catalan,shorthands=off}{babel}
\else
  \PassOptionsToPackage{english}{babel}
\fi



% Altres paquets necessaris
\RequirePackage[T1]{fontenc}
\RequirePackage{lmodern}
\RequirePackage{babel}
\RequirePackage{extras}
\RequirePackage{scrlayer-scrpage}
\RequirePackage{hyperref}
\RequirePackage{xcolor}
\RequirePackage{relsize}
%
% Preconfigura alguns elements
%

\def\@compromis{Amb la presentació d'aquest document els autors
  afirmen a tots els efectes que tant la pràctica informada com el
  propi informe és exclusivament fruit del seu treball personal i en
  cap cas s'ha plagiat total o parcialment.}

\newcommand{\titol}[1]{\title{Pràctica~\practno:~#1}}

\newcommand{\assignatura}[1]{%
  \subtitle{#1 --- \textsf{Enginyeria de Sistemes TIC}}
  \def\@assignatura{#1}
}

\newcommand{\autor}[1]{\author{#1}}

\newcommand{\numpract}[1]{\def\practno{#1}}
%comendes

%-------
% Coses matemàtiques
%-------
\newcommand{\laplace}[1]{\mathlarger{\mathlarger{\mathlarger{\mathcal{L}}}}\left\{ #1 \right\}}
\newcommand{\invlaplace}[1]{\mathlarger{\mathlarger{\mathlarger{\mathcal{L}^{-1}}}}\left\{ #1 \right\}}
\newcommand{\brac}[1]{\left ( #1 \right )}
\newcommand{\fourier}[1]{\mathlarger{\mathlarger{\mathlarger{\mathcal{F}}}}\left\{ #1 \right\}}
\newcommand{\invfourier}[1]{\mathlarger{\mathlarger{\mathlarger{\mathcal{F}^{-1}}}}\left\{ #1 \right\}}

\if@informe
\def\practica@firsthead{
  \raisebox{-4mm}[0pt][0pt]{%
    \kern-1in\kern-\oddsidemargin\kern0.05\paperwidth\parbox{0.9\paperwidth}%
    {\centering\tiny\textcolor{gray}\@compromis
    \par\medskip\logoepsem}
  }
}
\else
\def\practica@firsthead{
  \raisebox{7mm}[0pt][0pt]{%
    \kern-1in\kern-\oddsidemargin\kern0.05\paperwidth\parbox{0.9\paperwidth}%
    {\centering\tiny\textcolor{gray}{\ccbyncsa}
    \par\medskip\logoepsem}
  }
}
\fi

\clearscrplain
\clearscrheadfoot
\if@informe
\ifoot[Informe~\practno.~\@assignatura]{
  \def\and{, }\@author. Pràctica~\practno.~\@assignatura}
  \ofoot[\pagemark]{\pagemark}
  \chead[\practica@firsthead]{}
\else
  \ifoot[~\practno~\@assignatura]{~\practno~\@assignatura}
  \ofoot[\pagemark]{\pagemark}
  \chead[\practica@firsthead]{}
\fi
\pagestyle{scrheadings}

\setkomafont{pagehead}{\itshape\footnotesize}
\setkomafont{pagefoot}{\itshape\scriptsize}

\newenvironment{answer}{\textit{Answer} }{\textit{the end.}}

\AtBeginDocument{%
  %  nasty trick to avoid:
  %  https://www.mail-archive.com/tex-hyphen@tug.org/msg01801.html
  \if@practi@catalan\lccode`'=0\fi
  \sloppy
  \maketitle
  \tableofcontents
}
