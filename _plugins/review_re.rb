require 'review'
require 'fileutils'

module Jekyll
  class ReVIEWConverter < Converter
    safe true
    priority :low

    def matches(ext)
      ext =~ /^\.re$/i
    end

    def output_ext(ext)
      '.html'
    end

    def convert(content)
      f = File.open('/tmp/f.re', 'w')
      f.write(content)
      f.close()
      FileUtils.mkdir_p '/tmp/tmp'
      f = File.open('/tmp/tmp/f.re', 'w')
      f.write(content)
      f.close()

      config = ReVIEW::Configure.values
      config['builder'] = 'html'
      ReVIEW::I18n.setup(@config['language'])

      book = ReVIEW::Book::Base.load('/tmp')
      book.config = config
      chap = book.chapter('f')
      compiler = ReVIEW::Compiler.new(load_strategy_class('html', false))
      s = compiler.compile(chap)
      s = s.gsub('<span class="secno">chapterchapter_postfix</span>', '')
      s = s.gsub(/<span class="secno">\d+\.\d+chapter_postfix<\/span>/, '')
      s
    end

    def load_strategy_class(target, strict)
      require "review/#{target}builder"
      ReVIEW.const_get("#{target.upcase}Builder").new(strict)
    end
  end
end
