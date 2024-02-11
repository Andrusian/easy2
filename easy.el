;;; easy2-mode.el --- sample major mode for editing easy2 files -*- coding: utf-8; lexical-binding: t; -*-

;;; Commentary:

;; Colorizes easy2.e2 files in emacs

;; full doc on how to use here

;;; Code:

;; create the list for font-lock.
;; each category of keyword is given a particular face


(defvar easy2-mode-hook nil)

(defvar easy2-mode-map
  (let ((map (make-keymap)))
    (define-key map "\C-j" 'newline-and-indent)
    map)
  "Keymap for EASY2 major mode")

(defvar easy2-mode-syntax-table
  (let ((st (make-syntax-table)))
    (modify-syntax-entry ?_ "w" st)
    ;; comment style: "# ..."
    (modify-syntax-entry ?# "<" st)
    (modify-syntax-entry ?\n ">" st)
    (setq comment-start "# " ) 
    (setq comment-end "")
    st))




(setq easy2-font-lock-keywords
      (let* (
            (set-syntax-table easy2-mode-syntax-table)
            ;; define several category of keywords
            (x-keywords '("vol" "vol2" "vol3" "freq" "freq2" "freq3" "form" "phase" "bal" "cirp" "ciri" "duty" "automix" "circuit" "nocircuit" "manualmix" "fadeout" "fadein" "bal"))
            (x-types '("osc" "ramp" "ramps" "shape" "to" "seq" "randseq" ))
            (x-constants '("right" "left" "both" "sine" "square" "tri" "saw" "tens"))
            (x-events '("output" "exit" "time" "addtime" "rewind" "repeat" "macro" "loop"))
            (x-functions '("sound" "mix" "silence" "boost" "reverb"))

            ;; generate regex string for each category of keywords
            (x-keywords-regexp (regexp-opt x-keywords 'words))
            (x-types-regexp (regexp-opt x-types 'words))
            (x-constants-regexp (regexp-opt x-constants 'words))
            (x-events-regexp (regexp-opt x-events 'words))
            (x-functions-regexp (regexp-opt x-functions 'words)))

        `(
          (,x-types-regexp . 'font-lock-type-face)
          (,x-constants-regexp . 'font-lock-constant-face)
          (,x-events-regexp . 'font-lock-builtin-face)
          (,x-functions-regexp . 'font-lock-function-name-face)
          (,x-keywords-regexp . 'font-lock-keyword-face)
          ;; note: order above matters, because once colored, that part won't change.
          ;; in general, put longer words first
          )))




(defcustom easy2-indent-level 0
  "Indentation of Easy2 statements with respect to containing block."
  :type 'integer
  :group 'easy2-indentation-details)




;;;###autoload

(define-derived-mode easy2-mode c-mode "easy2 mode"
  "Major mode for editing EASY2 .e2 Files…"

  ;; code for syntax highlighting
  (setq font-lock-defaults '((easy2-font-lock-keywords))))

;; add the mode to the `features' list
(provide 'easy2-mode)

(add-to-list 'auto-mode-alist '("\\.e2$\\'" . easy2--mode))

(electric-indent-mode)

;;; easy2-mode.el ends here
