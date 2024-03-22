;; a simple major mode, mymath-mode

;; https://www.gnu.org/software/emacs/manual/html_node/elisp/Faces-for-Font-Lock.html

(setq jml-highlights
      '(("\\[macro" . 'font-lock-function-name-face)
        ("<h1>\\([^<]+?\\)</h1>" . (1 'font-lock-type-face))
        ("\\[macro \\([^\]\s]+\\)" . (1 'font-lock-type-face))
        ("\\[\\([^\]\/\s]+\\)" . (1 'font-lock-property-use-face))
	("@\\w+" 0 font-lock-string-face t)
	("\\$\\w+" 0 font-lock-string-face t)
	("\\[" 0 font-lock-keyword-face t)
	("\\]" 0 font-lock-keyword-face t)
	("\\[\\(\w+\))" 0 font-lock-warning-face t)
      ))

;;type ‘M-x list-faces-display’.  With a prefix argument, this prompts for



(define-derived-mode jml-mode fundamental-mode "jml"
  "major mode for editing jml language code."
  (setq font-lock-defaults '(jml-highlights)))
