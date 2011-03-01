C
C Modified by Jayanth Chennamangalam on 2010.03.21 for Fortran 95 compliance
C   Replaced the 'type' parameter in all 'open' statements with 'status', to
C   comply with Fortran 95.
C Modified by Jayanth Chennamangalam on 2011.02.20 to comment out unnecessary
C   write() statements.
C
c========================================================================
      subroutine set_colours(bw_plot,dmin,dmax)

      implicit none

      real*4 dmin,dmax
      logical*4 bw_plot,col_reverse
      integer*4 idx,jj,k,ibw,m,i0,ii,kk
      real*4 rgb(2,3),csr,csg,csb ,val1
      logical*4 col_file_found
      character*40 temp_string 

            call pgqcol(idx,jj)
C            write(*,*)'max # col:',jj
c first two colour indices used for background & default (txt) colour
            call pgqinf('HARDCOPY',temp_string,k)
C            write(*,'(a,a)')'HARDCOPY_DEVICE ? ....',
C     -                      temp_string(1:10)
            if(temp_string(1:3).eq.'YES')then
              col_reverse = .true.
            else
              col_reverse = .false.
            end if
c Set up the colour scale
            col_file_found = .false.
            if(.not.bw_plot)then
              if((dmin*dmax).ge.0.0)then    ! we have zero at one end
                if(dmin.ge.0.0)then
                  open(unit=35,
     -               file='colour_+ve_only.dat',status='old',
     -               err=1100)
                  col_file_found = .true. 
1100              if(.not.col_file_found)then
                    open(unit=35,
     -               file='colour_+ve_only.dat',status='unknown')
c==============================================================                    
                      write(35,*)'5,2,255'
                      write(35,*)'20'
                      write(35,*)'0.0,0.0,0.0'
                      write(35,*)'0.0,0.0,1.0'
                      write(35,*)'50'
                      write(35,*)'0.0,0.0,1.0'
                      write(35,*)'0.0,1.0,1.0'
                      write(35,*)'60'
                      write(35,*)'0.0,1.0,1.0'
                      write(35,*)'0.0,1.0,0.0'
                      write(35,*)'60'
                      write(35,*)'0.0,1.0,0.0'
                      write(35,*)'1.0,1.0,0.0'
                      write(35,*)'64'
                      write(35,*)'1.0,1.0,0.0'
                      write(35,*)'1.0,0.0,0.0'
                      write(35,*)' '
       write(35,*)' The first line contains no.of info-sets',
     -            ' (each 3-line)'
       write(35,*)' to follow,the offset colour index & ',
     -            ' the total no.'
       write(35,*)'of levels defined in the following sets.'
       write(35,*)' '
       write(35,*)'Each 3-line info has'
       write(35,*)' '
       write(35,*)'the no. of steps'
       write(35,*)'starting rgb values'
       write(35,*)'ending rgb values '
       write(35,*)' '
       write(35,*)' --- desh'
c==============================================================                    
                    close(unit=35)
                    open(unit=35,
     -               file='colour_+ve_only.dat',status='old')
                  end if
                else    ! -ve range
                  open(unit=35,
     -               file='colour_-ve_only.dat',status='old',
     -               err=1101)
                  col_file_found = .true. 
1101              if(.not.col_file_found)then
                    open(unit=35,
     -               file='colour_-ve_only.dat',status='unknown')
c==============================================================                    
                      write(35,*)'8,2,255'
                      write(35,*)'9'
                      write(35,*)'0.5,0.0,0.1'
                      write(35,*)'0.7,0.0,0.0'
                      write(35,*)'25  '
                      write(35,*)'0.7,0.0,0.0'
                      write(35,*)'0.7,0.2,0.0'
                      write(35,*)'40'
                      write(35,*)'0.7,0.2,0.0'
                      write(35,*)'1.0,1.0,0.0'
                      write(35,*)'40'
                      write(35,*)'1.0,1.0,0.0'
                      write(35,*)'0.7,0.7,0.0'
                      write(35,*)'40'
                      write(35,*)'0.7,0.7,0.0'
                      write(35,*)'0.0,0.5,0.0'
                      write(35,*)'40'
                      write(35,*)'0.0,0.5,0.0'
                      write(35,*)'0.0,0.7,0.7'
                      write(35,*)'40'
                      write(35,*)'0.0,0.7,0.7'
                      write(35,*)'0.1,0.1,0.7'
                      write(35,*)'20'
                      write(35,*)'0.1,0.1,0.7'
                      write(35,*)'0.0,0.0,0.0'
                      write(35,*)' '
       write(35,*)' The first line contains no.of info-sets',
     -            ' (each 3-line)'
       write(35,*)' to follow,the offset colour index & ',
     -            ' the total no.'
       write(35,*)'of levels defined in the following sets.'
       write(35,*)' '
       write(35,*)'Each 3-line info has'
       write(35,*)' '
       write(35,*)'the no. of steps'
       write(35,*)'starting rgb values'
       write(35,*)'ending rgb values '
       write(35,*)' '
       write(35,*)' --- desh'
c==============================================================                    
                    close(unit=35)
                    open(unit=35,
     -               file='colour_-ve_only.dat',status='old')
                  end if
                end if

              else     ! we have zero in the middle
c             make the range look symmetric
                val1 = dmax
                if(val1.lt.abs(dmin))val1 = abs(dmin)
                dmin = -val1
                dmax = val1
                open(unit=35,
     -               file='colour_+ve_to_-ve.dat',status='old',
     -               err=1102)
                col_file_found = .true. 
1102            if(.not.col_file_found)then
                  open(unit=35,
     -               file='colour_+ve_to_-ve.dat',status='unknown')
c==============================================================                    
                  write(35,*)'8,2,255'
                  write(35,*)'30'
                  write(35,*)'0.0,0.5,0.0'
                  write(35,*)'0.0,0.9,0.0'
                  write(35,*)'30'
                  write(35,*)'0.0,0.9,0.0'
                  write(35,*)'0.0,1.0,1.0'
                  write(35,*)'40'
                  write(35,*)'0.0,1.0,1.0'
                  write(35,*)'0.0,0.0,1.0'
                  write(35,*)'27'
                  write(35,*)'0.0,0.0,1.0'
                  write(35,*)'0.0,0.0,0.0'
                  write(35,*)'47'
                  write(35,*)'0.0,0.0,0.0'
                  write(35,*)'1.0,1.0,0.0'
                  write(35,*)'50'
                  write(35,*)'1.0,1.0,0.0'
                  write(35,*)'1.0,0.0,0.0'
                  write(35,*)'15'
                  write(35,*)'1.0,0.0,0.0'
                  write(35,*)'0.7,0.0,0.0'
                  write(35,*)'15'
                  write(35,*)'0.7,0.0,0.0'
                  write(35,*)'0.5,0.0,0.1'
                  write(35,*)' '
       write(35,*)' The first line contains no.of info-sets',
     -            ' (each 3-line)'
       write(35,*)' to follow,the offset colour index & ',
     -            ' the total no.'
       write(35,*)' of levels defined in the following sets.'
       write(35,*)' '
       write(35,*)' Each 3-line info has '
       write(35,*)' '
       write(35,*)' the no. of steps'
       write(35,*)' starting rgb values'
       write(35,*)' ending rgb values'
       write(35,*)' -----------------------------desh '
c==============================================================                    
                  close(unit=35)
                  open(unit=35,
     -               file='colour_+ve_to_-ve.dat',status='old')
                end if
              end if
            else           ! gray scale
                open(unit=35,
     -               file='gray_scale.dat',status='old',
     -               err=1103)
                col_file_found = .true. 
1103            if(.not.col_file_found)then
                  open(unit=35,
     -               file='gray_scale.dat',status='unknown')
c==============================================================                    
                  write(35,*)'2,2,255'
                  write(35,*)'128'
                  write(35,*)'0.0,0.0,0.0'
                  write(35,*)'0.5,0.5,0.5'
                  write(35,*)'127'
                  write(35,*)'0.5,0.5,0.5'
                  write(35,*)'1.0,1.0,1.0'
                  write(35,*)' '
       write(35,*)' The first line contains no.of info-sets',
     -            ' (each 3-line)'
       write(35,*)' to follow,the offset colour index & ',
     -            ' the total no.'
       write(35,*)' of levels defined in the following sets.'
       write(35,*)' '
       write(35,*)' Each 3-line info has '
       write(35,*)' '
       write(35,*)' the no. of steps'
       write(35,*)' starting rgb values'
       write(35,*)' ending rgb values'
       write(35,*)' -----------------------------desh '
c==============================================================                    
                  close(unit=35)
                  open(unit=35,
     -               file='gray_scale.dat',status='old')
              end if
            end if

            read(35,*,err=1111)ibw,ii,kk
            if(ii.lt.2)ii = 2
            k = ii
            do m=1,ibw
              read(35,*,err=1111)i0
              i0 = real(i0)*real(jj-2)/real(kk-ii)

              do idx=1,2     ! read the start and stop settings
                read(35,*,err=1111)
     -               rgb(idx,1),rgb(idx,2),rgb(idx,3)
                val1 = rgb(idx,1) + rgb(idx,2) + rgb(idx,3)
                if(val1.eq.0.0.or.val1.eq.3.0)then   !we have black or white
                  if(col_reverse)then
                    rgb(idx,1) = 1. - rgb(idx,1)
                    rgb(idx,2) = 1. - rgb(idx,2)
                    rgb(idx,3) = 1. - rgb(idx,3)
                  end if
                end if
              end do

              if(i0.eq.0)i0=1
              do idx=1,3
                rgb(2,idx) = (rgb(2,idx)-rgb(1,idx))/
     -                       real(i0)
              end do
              do idx=1,i0
                val1 = real(idx-1)
                csr = rgb(1,1) + val1*rgb(2,1)
                csg = rgb(1,2) + val1*rgb(2,2)
                csb = rgb(1,3) + val1*rgb(2,3)
                if(k.gt.1.and.k.le.jj)then
                  call pgscr(k,csr,csg,csb)
                  k = k + 1
                end if
              end do
            end do
            k = k - 1
            if(k.lt.jj)jj = k

            call pgscir(ii,jj)
            close(unit=35,err=1112)
      return
1111  write(*,*)' Error in reading colour .dat files '
      write(*,*)' in the present directory...'
      write(*,*)' Please copy  ....'
      write(*,*)' colour_+ve_only.dat'
      write(*,*)' colour_-ve_only.dat'
      write(*,*)' colour_+ve_to_-ve.dat'
      write(*,*)' gray_scale.dat'
      write(*,*)'================================'
      return
1112  close(unit=35,err=1113)
      return
1113  write(*,*)' Error during closing the colour-code file.'
      return
      end

